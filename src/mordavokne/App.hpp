#pragma once

#include <memory>

#include <utki/config.hpp>
#include <utki/Singleton.hpp>
#include <utki/Buf.hpp>
#include <utki/Unique.hpp>
#include <utki/Flags.hpp>

#include <papki/File.hpp>

#include <kolme/Vector2.hpp>

#include <morda/Morda.hpp>

#include <morda/util/keycodes.hpp>

#include "config.hpp"



//TODO:remove
#if M_OS == M_OS_WINDOWS
#	include <utki/windows.hpp>
#endif




namespace mordavokne{



/**
 * @brief Base singleton class of Application.
 * An application should subclass this class and return an instance from the
 * application factory function createApp(), see AppFactory.hpp for details.
 * When instance of this class is created it also creates a window and
 * initializes rendering API (e.g. OpenGL or OpenGL ES).
 */
class App :
		public utki::IntrusiveSingleton<App>,
		public utki::Unique
{
	friend T_Singleton;
	static T_Instance instance;
	
	
private:

public:	
	/**
	 * @brief Desired window parameters.
	 */
	struct WindowParams{
		/**
		 * @brief Desired dimensions of the window
		 */
		kolme::Vec2ui dim;
		
		enum class Buffer_e{
			DEPTH,
			STENCIL,
			
			ENUM_SIZE
		};
		
		/**
		 * @brief Flags describing desired buffers for OpneGL context.
		 */
		utki::Flags<Buffer_e> buffers = utki::Flags<Buffer_e>(false);
		
		WindowParams(kolme::Vec2ui dim) :
				dim(dim)
		{}
	};
	
private:
	std::unique_ptr<utki::Unique> windowPimpl;
	
	friend const decltype(windowPimpl)& getWindowPimpl(App& app);

#if M_OS == M_OS_LINUX


#	if M_OS_NAME == M_OS_NAME_ANDROID
private:
	friend void updateWindowRect(App& app, const morda::Rectr& rect);
	friend void Render(App& app);
	friend std::uint32_t Update(App& app);
	friend void HandleInputEvents();
	friend void HandleCharacterInputEvent(std::vector<std::uint32_t>&& chars);
	friend void HandleQueueMessages(App& app);
	friend int GetUIQueueHandle(App& app);

private:
	struct EGLDisplayWrapper{
		EGLDisplay d;
		EGLDisplayWrapper();
		~EGLDisplayWrapper()noexcept;
	} eglDisplay;
	
	struct EGLConfigWrapper{
		EGLConfig c;
		EGLConfigWrapper(const WindowParams& wp, EGLDisplayWrapper& d);
		~EGLConfigWrapper()noexcept{}
	} eglConfig;
	
	struct EGLSurfaceWrapper{
		EGLDisplayWrapper& d;
		EGLSurface s;
		EGLSurfaceWrapper(EGLDisplayWrapper&d, EGLConfigWrapper& c);
		~EGLSurfaceWrapper()noexcept;
	} eglSurface;
	
	struct EGLContextWrapper{
		EGLDisplayWrapper& d;
		EGLContext c;
		EGLContextWrapper(EGLDisplayWrapper& d, EGLConfigWrapper& config, EGLSurfaceWrapper& s);
		~EGLContextWrapper()noexcept;
	} eglContext;
#	endif

#elif M_OS == M_OS_WINDOWS

private:
	struct WindowClassWrapper{
		std::string name;

		WindowClassWrapper();
		~WindowClassWrapper()noexcept;
	} windowClass;

	struct WindowWrapper{
		HWND hwnd;

		bool isHovered = false; //for tracking when mouse enters or leaves window.

		utki::Flags<morda::MouseButton_e> mouseButtonState;

		WindowWrapper(const WindowParams& wp, const WindowClassWrapper& wc);
		~WindowWrapper()noexcept;
	} window;

	struct DeviceContextWrapper{
		const WindowWrapper& w;
		HDC hdc;

		DeviceContextWrapper(const WindowParams& wp, const WindowWrapper& w);
		~DeviceContextWrapper()noexcept{
			this->Destroy();
		}

	private:
		void Destroy()noexcept;
	} deviceContext;


	struct OpenGLContextWrapper{
		HGLRC hrc;
	public:
		OpenGLContextWrapper(HDC hdc);

		~OpenGLContextWrapper()noexcept {
			this->Destroy();
		}

		void Destroy() noexcept;
	} openglContext;




	bool mouseCursorIsCurrentlyVisible = true;
	
	friend void winmain(int argc, const char** argv);
	void exec();
	void postToUiThread_ts(std::function<void()>&& f);
	friend bool handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& lres);

#elif M_OS == M_OS_MACOSX

#else
#	error "unsupported OS"
#endif

private:
	void swapFrameBuffers();

public:
	class MordaVOkne : public morda::Morda{
		App& app;
	public:
		MordaVOkne(App& app, std::shared_ptr<morda::Renderer> r, morda::real dotsPerInch, morda::real dotsPerPt) :
				Morda(r, dotsPerInch, dotsPerPt),
				app(app)
		{
			TRACE(<< "MordaVOkne::MordaVOkne(): enter" << std::endl)
		}
		

		void postToUiThread_ts(std::function<void()>&& f) override;
//TODO: move to glue
//		{
//#if M_OS == M_OS_WINDOWS || M_OS == M_OS_MACOSX
//			App::inst().postToUiThread_ts(std::move(f));
//#else
//			App::inst().uiQueue.pushMessage(std::move(f));
//#endif
//		}
	} gui;
	
public:

	/**
	 * @brief Create file interface into resources storage.
	 * This function creates a morda's standard file interface to read application's
	 * resources.
	 * @param path - file path to initialize the file interface with.
	 * @return Instance of the file interface into the resources storage.
	 */
	std::unique_ptr<papki::File> createResourceFileInterface(const std::string& path = std::string())const;
	
	
private:
	//this is a viewport rectangle in coordinates that are as follows: x grows right, y grows up.
	morda::Rectr curWinRect = morda::Rectr(0, 0, 0, 0);

public:
	const morda::Vec2r& winDim()const noexcept{
		return this->curWinRect.d;
	}

private:
	void render();
	
	friend void render(App& app);
	
	void updateWindowRect(const morda::Rectr& rect);

	friend void updateWindowRect(App& app, const morda::Rectr& rect);
	
	//pos is in usual window coordinates, y goes down.
	morda::Vec2r nativeWindowToRootCoordinates(const kolme::Vec2f& pos)const noexcept{
		return morda::Vec2r(pos.x, this->curWinRect.d.y - pos.y - 1.0f);
	}

	//pos is in usual window coordinates, y goes down.
	void handleMouseMove(const kolme::Vec2f& pos, unsigned id){
		this->gui.onMouseMove(this->nativeWindowToRootCoordinates(pos), id);
	}
	
	friend void handleMouseMove(App& app, const kolme::Vec2f& pos, unsigned id);

	//pos is in usual window coordinates, y goes down.
	void handleMouseButton(bool isDown, const kolme::Vec2f& pos, morda::MouseButton_e button, unsigned id){
		this->gui.onMouseButton(isDown, this->nativeWindowToRootCoordinates(pos), button, id);
	}
	
	friend void handleMouseButton(App& app, bool isDown, const kolme::Vec2f& pos, morda::MouseButton_e button, unsigned id);

	void handleMouseHover(bool isHovered, unsigned pointerID){
		this->gui.onMouseHover(isHovered, pointerID);
	}
	
	friend void handleMouseHover(App& app, bool isHovered, unsigned pointerID);

protected:
	/**
	 * @brief Application constructor.
	 * @param requestedWindowParams - requested window parameters.
	 */
	App(const WindowParams& requestedWindowParams);

public:

	virtual ~App()noexcept{}

	/**
	 * @brief Bring up the virtual keyboard.
	 * On mobile platforms this function will summon the on-screen keyboard.
	 * On desktop platforms this function does nothing.
	 */
	void showVirtualKeyboard()noexcept;

	/**
	 * @brief Hide virtual keyboard.
	 * On mobile platforms this function hides the on-screen keyboard.
	 * On desktop platforms this function does nothing.
	 */
	void hideVirtualKeyboard()noexcept;

private:
	

	//The idea with UnicodeResolver parameter is that we don't want to calculate the unicode unless it is really needed, thus postpone it
	//as much as possible.
	void handleCharacterInput(const morda::Morda::UnicodeProvider& unicodeResolver, morda::Key_e key){
		this->gui.onCharacterInput(unicodeResolver, key);
	}
	
	friend void handleCharacterInput(App& app, const morda::Morda::UnicodeProvider& unicodeResolver, morda::Key_e key);

	void handleKeyEvent(bool isDown, morda::Key_e keyCode){
		this->gui.onKeyEvent(isDown, keyCode);
	}
	
	friend void handleKeyEvent(App& app, bool isDown, morda::Key_e keyCode);

public:
	
	/**
	 * @brief Requests application to exit.
	 * This function posts an exit message to the applications message queue.
	 * The message will normally be handled on the next UI cycle and as a result
	 * the main loop will be terminated and application will exit. The Application
	 * object will be destroyed and all resources freed.
	 */
	void quit()noexcept;

private:
	bool isFullscreen_v = false;

	kolme::Rectu beforeFullScreenWindowRect;

public:
	/**
	 * @brief Check if application currently runs in fullscreen mode.
	 * @return true if application is in fullscreen mode.
	 * @return false if application is in windowed mode.
	 */
	bool isFullscreen()const noexcept {
		return this->isFullscreen_v;
	}

	/**
	 * @brief Set/unset fullscreen mode.
	 * @param enable - whether to enable or to disable fullscreen mode.
	 */
	void setFullscreen(bool enable);
	

	/**
	 * @brief Show/hide mouse cursor.
	 * @param visible - whether to show (true) or hide (false) mouse cursor.
	 */
	void setMouseCursorVisible(bool visible);
	
	
	/**
	 * @brief Find dots per point for given display parameters.
	 * The size of the point for desktop displays should normally be equal to one pixel.
	 * For handheld devices size of the point depends on physical screen size and pixel resolution.
	 * @param resolution - resolution of the display in pixels.
	 * @param screenSizeMm - size of the display in millimeters.
	 * @return Size of one display point in pixels.
	 */
	static morda::real findDotsPerPt(kolme::Vec2ui resolution, kolme::Vec2ui screenSizeMm);
};

inline App& inst(){
	return App::inst();
}

}
