/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2025  Ivan Gagis <igagis@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* ================ LICENSE END ================ */

#pragma once

#include <android/asset_manager.h>
#include <fsif/file.hpp>

namespace {
class asset_file : public fsif::file
{
	AAssetManager* manager;

	mutable AAsset* handle = nullptr;

public:
	asset_file(
		AAssetManager* manager, //
		std::string_view path_name = std::string_view()
	) :
		manager(manager),
		fsif::file(path_name)
	{
		utki::assert(this->manager, SL);
	}

	virtual void open_internal(fsif::mode mode) override
	{
		switch (mode) {
			case fsif::mode::write:
			case fsif::mode::create:
				throw std::invalid_argument(
					"'write' and 'create' open modes are not "
					"supported by Android assets"
				);
			case fsif::mode::read:
				break;
			default:
				throw std::invalid_argument("unknown mode");
		}
		this->handle = AAssetManager_open(
			this->manager,
			this->path().c_str(),
			AASSET_MODE_UNKNOWN // don't know what this MODE means at all
		);
		if (!this->handle) {
			std::stringstream ss;
			ss << "AAssetManager_open(" << this->path() << ") failed";
			throw std::runtime_error(ss.str());
		}
	}

	virtual void close_internal() const noexcept override
	{
		utki::assert(this->handle, SL);
		AAsset_close(this->handle);
		this->handle = 0;
	}

	virtual size_t read_internal(utki::span<std::uint8_t> buf) const override
	{
		utki::assert(this->handle, SL);
		int num_bytes_read = AAsset_read(
			this->handle, //
			buf.data(),
			buf.size()
		);
		if (num_bytes_read < 0) {
			throw std::runtime_error("AAsset_read() error");
		}
		utki::assert(num_bytes_read >= 0, SL);
		return size_t(num_bytes_read);
	}

	virtual size_t write_internal(utki::span<const uint8_t> buf) override
	{
		utki::assert(this->handle, SL);
		throw std::runtime_error("write() is not supported by Android assets");
	}

	virtual size_t seek_forward_internal(size_t num_bytes_to_seek) const override
	{
		return this->seek(num_bytes_to_seek, true);
	}

	virtual size_t seek_backward_internal(size_t num_bytes_to_seek) const override
	{
		return this->seek(num_bytes_to_seek, false);
	}

	virtual void rewind_internal() const override
	{
		if (!this->is_open()) {
			throw std::logic_error("file is not opened, cannot rewind");
		}

		utki::assert(this->handle, SL);
		if (AAsset_seek(this->handle, 0, SEEK_SET) < 0) {
			throw std::runtime_error("AAsset_seek() failed");
		}
	}

	virtual bool exists() const override
	{
		if (this->is_open()) {
			// file is opened => it exists
			return true;
		}

		if (this->path().size() == 0) {
			return false;
		}

		if (this->is_dir()) {
			// try opening the directory to check its existence
			AAssetDir* pdir = AAssetManager_openDir(
				this->manager, //
				this->path().c_str()
			);

			if (!pdir) {
				return false;
			} else {
				AAssetDir_close(pdir);
				return true;
			}
		} else {
			return this->file::exists();
		}
	}

	virtual std::vector<std::string> list_dir(size_t max_num_entries = 0) const override
	{
		if (!this->is_dir()) {
			throw std::logic_error("asset_file::list_dir(): this is not a directory");
		}

		// Trim away trailing '/', as Android does not work with it.
		auto p = this->path().substr(
			0, //
			this->path().size() - 1
		);

		auto& glob = get_glob();

		return glob.java_functions.list_dir_contents(p);
	}

	std::unique_ptr<fsif::file> spawn() override
	{
		return std::make_unique<asset_file>(this->manager);
	}

	~asset_file() noexcept {}

	size_t seek(
		size_t num_bytes_to_seek, //
		bool seek_forward
	) const
	{
		if (!this->is_open()) {
			throw std::logic_error("file is not opened, cannot seek");
		}

		utki::assert(this->handle, SL);

		// NOTE: AAsset_seek() accepts 'off_t' as offset argument which is signed
		//       and can be less than size_t value passed as argument to this function.
		//       Therefore, do several seek operations with smaller offset if
		//       necessary.

		off_t asset_size = AAsset_getLength(this->handle);
		utki::assert(asset_size >= 0, SL);

		using std::min;
		if (seek_forward) {
			utki::assert(size_t(asset_size) >= this->cur_pos(), SL);
			num_bytes_to_seek = min(num_bytes_to_seek, size_t(asset_size) - this->cur_pos()); // clamp top
		} else {
			num_bytes_to_seek = min(num_bytes_to_seek, this->cur_pos()); // clamp top
		}

		typedef off_t seek_offset_type;
		const size_t max_seek = ((size_t(seek_offset_type(-1))) >> 1);
		utki::assert((size_t(1) << ((sizeof(seek_offset_type) * utki::byte_bits) - 1)) - 1 == max_seek, SL);
		static_assert(size_t(-(-seek_offset_type(max_seek))) == max_seek, "size mismatch");

		for (size_t num_bytes_left = num_bytes_to_seek; num_bytes_left != 0;) {
			utki::assert(num_bytes_left <= num_bytes_to_seek, SL);

			seek_offset_type offset;
			if (num_bytes_left > max_seek) {
				offset = seek_offset_type(max_seek);
			} else {
				offset = seek_offset_type(num_bytes_left);
			}

			utki::assert(offset > 0, SL);

			if (AAsset_seek(
					this->handle, //
					seek_forward ? offset : (-offset),
					SEEK_CUR // relative to the current file position
				) < 0)
			{
				throw std::runtime_error("AAsset_seek() failed");
			}

			utki::assert(size_t(offset) < size_t(-1), SL);
			utki::assert(num_bytes_left >= size_t(offset), SL);

			num_bytes_left -= size_t(offset);
		}
		return num_bytes_to_seek;
	}
};
} // namespace
