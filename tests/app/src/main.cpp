#include <kolme/Quaternion.hpp>
#include <utki/debug.hpp>
#include <utki/config.hpp>
#include <papki/FSFile.hpp>

#include "../../../src/mordavokne/AppFactory.hpp"

#include <morda/widgets/Widget.hpp>
#include <morda/widgets/Container.hpp>
#include <morda/widgets/proxy/KeyProxy.hpp>

#include <morda/widgets/button/Button.hpp>
#include <morda/widgets/button/PushButton.hpp>
#include <morda/widgets/label/Text.hpp>

#include <morda/res/ResTexture.hpp>
#include <morda/res/ResFont.hpp>

#include <morda/widgets/CharInputWidget.hpp>
#include <morda/widgets/group/ScrollArea.hpp>
#include <morda/widgets/group/Row.hpp>
#include <morda/widgets/slider/ScrollBar.hpp>
#include <morda/widgets/group/List.hpp>
#include <morda/widgets/group/TreeView.hpp>
#include <morda/widgets/proxy/MouseProxy.hpp>
#include <morda/widgets/proxy/ResizeProxy.hpp>
#include <morda/widgets/label/Color.hpp>
#include <morda/widgets/label/Image.hpp>

#include <morda/util/ZipFile.hpp>


#if M_OS_NAME == M_OS_NAME_IOS
#	include <OpenGlES/ES2/glext.h>
#else
//#	include <GL/glew.h>
#endif


class SimpleWidget : virtual public morda::Widget, public morda::Updateable, public morda::CharInputWidget{
	std::shared_ptr<morda::ResTexture> tex;
	
public:	
	SimpleWidget(const stob::Node* desc) :
			morda::Widget(desc)
	{
//		TRACE(<< "loading texture" << std::endl)
		this->tex = morda::Morda::inst().resMan.load<morda::ResTexture>("tex_sample");
	}
	
	std::uint32_t timer = 0;
	std::uint32_t cnt = 0;
	
	void update(std::uint32_t dt) override{
		this->timer += dt;
		++this->cnt;
		
		if(this->timer > 1000){
			this->timer -= 1000;
			
			TRACE(<< "Update(): UPS = " << this->cnt << std::endl)
			
			this->cnt = 0;
		}
	}
	
	bool onMouseButton(bool isDown, const morda::Vec2r& pos, morda::MouseButton_e button, unsigned pointerId) override{
		TRACE(<< "OnMouseButton(): isDown = " << isDown << ", pos = " << pos << ", button = " << unsigned(button) << ", pointerId = " << pointerId << std::endl)
		if(!isDown){
			return false;
		}
		
		if(this->isUpdating()){
			this->stopUpdating();
		}else{
			this->startUpdating(30);
		}
		this->focus();
		return true;
	}
	
	bool onKey(bool isDown, morda::Key_e keyCode) override{
		if(isDown){
			TRACE(<< "SimpleWidget::OnKey(): down, keyCode = " << unsigned(keyCode) << std::endl)
			switch(keyCode){
				case morda::Key_e::LEFT:
					TRACE(<< "SimpleWidget::OnKeyDown(): LEFT key caught" << std::endl)
					return true;
				case morda::Key_e::A:
					TRACE(<< "SimpleWidget::OnKeyUp(): A key caught" << std::endl)
					return true;
				default:
					break;
			}
		}else{
			TRACE(<< "SimpleWidget::OnKey(): up, keyCode = " << unsigned(keyCode) << std::endl)
			switch(keyCode){
				case morda::Key_e::LEFT:
					TRACE(<< "SimpleWidget::OnKeyUp(): LEFT key caught" << std::endl)
					return true;
				case morda::Key_e::A:
					TRACE(<< "SimpleWidget::OnKeyUp(): A key caught" << std::endl)
					return true;
				default:
					break;
			}
		}
		return false;
	}
	
	void onCharacterInput(const std::u32string& unicode, morda::Key_e key) override{
		if(unicode.size() == 0){
			return;
		}
		
		TRACE(<< "SimpleWidget::OnCharacterInput(): unicode = " << unicode[0] << std::endl)
	}
	
	void render(const morda::Matr4r& matrix)const override{
		morda::Matr4r matr(matrix);
		matr.scale(this->rect().d);

		auto& r = morda::inst().renderer();
		r.shader->posTex->render(matr, *r.posTexQuad01VAO, this->tex->tex());
	}
};



class CubeWidget : public morda::Widget, public morda::Updateable{
	std::shared_ptr<morda::ResTexture> tex;
	
	morda::Quatr rot = morda::Quatr().identity();
public:
	std::shared_ptr<morda::VertexArray> cubeVAO;
	
	CubeWidget(const stob::Node* desc) :
			Widget(desc)
	{
		std::array<morda::Vec3r, 36> cubePos = {{
			kolme::Vec3f(-1, -1, 1), kolme::Vec3f(1, -1, 1), kolme::Vec3f(-1, 1, 1),
			kolme::Vec3f(1, -1, 1), kolme::Vec3f(1, 1, 1), kolme::Vec3f(-1, 1, 1),
			
			kolme::Vec3f(1, -1, 1), kolme::Vec3f(1, -1, -1), kolme::Vec3f(1, 1, 1),
			kolme::Vec3f(1, -1, -1), kolme::Vec3f(1, 1, -1), kolme::Vec3f(1, 1, 1),
			
			kolme::Vec3f(1, -1, -1), kolme::Vec3f(-1, -1, -1), kolme::Vec3f(1, 1, -1),
			kolme::Vec3f(-1, -1, -1), kolme::Vec3f(-1, 1, -1), kolme::Vec3f(1, 1, -1),
			
			kolme::Vec3f(-1, -1, -1), kolme::Vec3f(-1, -1, 1), kolme::Vec3f(-1, 1, -1),
			kolme::Vec3f(-1, -1, 1), kolme::Vec3f(-1, 1, 1), kolme::Vec3f(-1, 1, -1),
			
			kolme::Vec3f(-1, 1, -1), kolme::Vec3f(-1, 1, 1), kolme::Vec3f(1, 1, -1),
			kolme::Vec3f(-1, 1, 1), kolme::Vec3f(1, 1, 1), kolme::Vec3f(1, 1, -1),
			
			kolme::Vec3f(-1, -1, -1), kolme::Vec3f(1, -1, -1), kolme::Vec3f(-1, -1, 1),
			kolme::Vec3f(-1, -1, 1), kolme::Vec3f(1, -1, -1), kolme::Vec3f(1, -1, 1)
		}};
		
		auto posVBO = morda::inst().renderer().factory->createVertexBuffer(utki::wrapBuf(cubePos));
		
		std::array<kolme::Vec2f, 36> cubeTex = {{
			kolme::Vec2f(0, 0), kolme::Vec2f(1, 0), kolme::Vec2f(0, 1),
			kolme::Vec2f(1, 0), kolme::Vec2f(1, 1), kolme::Vec2f(0, 1),
			
			kolme::Vec2f(0, 0), kolme::Vec2f(1, 0), kolme::Vec2f(0, 1),
			kolme::Vec2f(1, 0), kolme::Vec2f(1, 1), kolme::Vec2f(0, 1),
			
			kolme::Vec2f(0, 0), kolme::Vec2f(1, 0), kolme::Vec2f(0, 1),
			kolme::Vec2f(1, 0), kolme::Vec2f(1, 1), kolme::Vec2f(0, 1),
		
			kolme::Vec2f(0, 0), kolme::Vec2f(1, 0), kolme::Vec2f(0, 1),
			kolme::Vec2f(1, 0), kolme::Vec2f(1, 1), kolme::Vec2f(0, 1),
			
			kolme::Vec2f(0, 0), kolme::Vec2f(1, 0), kolme::Vec2f(0, 1),
			kolme::Vec2f(1, 0), kolme::Vec2f(1, 1), kolme::Vec2f(0, 1),
			
			kolme::Vec2f(0, 0), kolme::Vec2f(1, 0), kolme::Vec2f(0, 1),
			kolme::Vec2f(1, 0), kolme::Vec2f(1, 1), kolme::Vec2f(0, 1)
		}};
		
		auto texVBO = morda::inst().renderer().factory->createVertexBuffer(utki::wrapBuf(cubeTex));
		
		std::array<std::uint16_t, 36> indices = {{
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
		}};
		
		auto cubeIndices = morda::inst().renderer().factory->createIndexBuffer(utki::wrapBuf(indices));
		
		this->cubeVAO = morda::inst().renderer().factory->createVertexArray({posVBO, texVBO}, cubeIndices, morda::VertexArray::Mode_e::TRIANGLES);
		
		this->tex = morda::Morda::inst().resMan.load<morda::ResTexture>("tex_sample");
		this->rot.identity();
		
		
	}
	
	unsigned fps = 0;
	std::uint32_t fpsSecCounter = 0;
	
	void update(std::uint32_t dt) override{
		this->fpsSecCounter += dt;
		++this->fps;
		this->rot %= morda::Quatr().initRot(kolme::Vec3f(1, 2, 1).normalize(), 1.5f * (float(dt) / 1000));
		if(this->fpsSecCounter >= 1000){
			TRACE_ALWAYS(<< "fps = " << fps << std::endl)
			this->fpsSecCounter = 0;
			this->fps = 0;
		}
	}
	
	void render(const morda::Matr4r& matrix)const override{
		this->Widget::render(matrix);
		
		morda::Matr4r matr(matrix);
		matr.scale(this->rect().d / 2);
		matr.translate(1, 1);
		matr.frustum(-2, 2, -1.5, 1.5, 2, 100);
		
		morda::Matr4r m(matr);
		m.translate(0, 0, -4);
		
		m.rotate(this->rot);

//		glEnable(GL_CULL_FACE);
		
		morda::inst().renderer().shader->posTex->render(m, *this->cubeVAO, this->tex->tex());
		
//		glDisable(GL_CULL_FACE);
	}
};



class TreeViewItemsProvider : public morda::TreeView::ItemsProvider{
	std::unique_ptr<stob::Node> root;
	
public:
	
	TreeViewItemsProvider(){
		this->root = stob::parse(R"qwertyuiop(
				{
					root1{
						subroot1{
							subsubroot1
							subsubroot2
							subsubroot3
							subsubroot4
						}
						subroot2
						subroot3{
							subsubroot0
							subsubroot1{
								subsubsubroot1
								subsubsubroot2
							}
							subsubroot2
						}
					}
					root2{
						subsubroot1
						subsubroot2{
							trololo
							"hello world!"
						}
					}
					root3
					root4
				}
			)qwertyuiop");
	}
	
	~TreeViewItemsProvider(){
		
	}
	
	const char* DPlusMinus = R"qwertyuiop(
			Pile{
				Image{
					name{plusminus}
				}
				MouseProxy{
					layout{
						dx{fill} dy{fill}
					}
					name{plusminus_mouseproxy}
				}
			}
		)qwertyuiop";
	
	const char* DLine = R"qwertyuiop(
			Pile{
				layout{dx{5mm} dy{fill}}
				Color{
					layout{dx{1pt}dy{fill}}
					color{@{morda_color_highlight}}
				}
			}
		)qwertyuiop";
	
	const char* DLineEnd = R"qwertyuiop(
			Pile{
				layout{dx{5mm} dy{max}}
				Column{
					layout{dx{max}dy{max}}
					Color{
						layout{dx{1pt}dy{0}weight{1}}
						color{@{morda_color_highlight}}
					}
					Widget{layout{dx{max}dy{0}weight{1}}}
				}
				Row{
					layout{dx{max}dy{max}}
					Widget{layout{dx{0}dy{max}weight{1}}}
					Color{
						layout{dx{0}dy{1pt}weight{1}}
						color{@{morda_color_highlight}}
					}
				}
			}
		)qwertyuiop";
	
	const char* DLineMiddle = R"qwertyuiop(
			Pile{
				layout{dx{5mm} dy{max}}
				Color{
					layout{dx{1pt}dy{max}}
					color{@{morda_color_highlight}}
				}
				Row{
					layout{dx{max}dy{max}}
					Widget{layout{dx{0}dy{max}weight{1}}}
					Color{
						layout{dx{0}dy{1pt}weight{1}}
						color{@{morda_color_highlight}}
					}
				}
			}
		)qwertyuiop";
	
	const char* DEmpty = R"qwertyuiop(
			Widget{layout{dx{5mm}dy{0}}}
		)qwertyuiop";
	
private:
	std::vector<size_t> selectedItem;
	
	unsigned newItemNumber = 0;
	
	std::string generateNewItemvalue(){
		std::stringstream ss;
		ss << "newItem" << newItemNumber;
		++newItemNumber;
		return ss.str();
	}
	
public:
	void insertBefore(){
		if(this->selectedItem.size() == 0){
			return;
		}
		
		auto n = this->root.get();
		if(!n){
			return;
		}
		
		decltype(n) parent = nullptr;
		decltype(n) prev = nullptr;
		
		for(auto i = this->selectedItem.begin(); n && i != this->selectedItem.end(); ++i){
			parent = n;
			auto next = n->child(*i);
			
			n = next.node();
			prev = next.prev();
		}
		
		if(!n){
			return;
		}
		
		if(prev){
			prev->insertNext(utki::makeUnique<stob::Node>(this->generateNewItemvalue().c_str()));
		}else{
			parent->addAsFirstChild(this->generateNewItemvalue().c_str());
		}
		
		this->notifyItemAdded(this->selectedItem);
		++this->selectedItem.back();
	}
	
	void insertAfter(){
		if(this->selectedItem.size() == 0){
			return;
		}
		
		auto n = this->root.get();
		if(!n){
			return;
		}
		
		for(auto i = this->selectedItem.begin(); n && i != this->selectedItem.end(); ++i){
			auto next = n->child(*i);
			n = next.node();
		}
		
		if(!n){
			return;
		}
		
		n->insertNext(utki::makeUnique<stob::Node>(this->generateNewItemvalue().c_str()));
		
		++this->selectedItem.back();
		this->notifyItemAdded(this->selectedItem);
		--this->selectedItem.back();
	}
	
	void insertChild(){
		auto n = this->root.get();
		if(!n){
			return;
		}
		
		for(auto i = this->selectedItem.begin(); n && i != this->selectedItem.end(); ++i){
			auto next = n->child(*i);
			n = next.node();
		}
		
		if(!n || this->selectedItem.size() == 0){
			this->selectedItem.clear();
			n = this->root.get();
		}
		
		n->addAsFirstChild(this->generateNewItemvalue().c_str());
		
		this->selectedItem.push_back(0);
		this->notifyItemAdded(this->selectedItem);
		this->selectedItem.pop_back();
	}
	
	std::shared_ptr<morda::Widget> getWidget(const std::vector<size_t>& path, bool isCollapsed)override{
		ASSERT(path.size() >= 1)
		
		auto n = this->root.get();
		decltype(n) parent = nullptr;
		
		std::vector<bool> isLastItemInParent;
		
		for(auto i = path.begin(); i != path.end(); ++i){
			parent = n;
			n = n->child(*i).node();
			isLastItemInParent.push_back(n->next() == nullptr);
		}
		
		auto ret = utki::makeShared<morda::Row>(nullptr);

		ASSERT(isLastItemInParent.size() == path.size())
		
		for(unsigned i = 0; i != path.size() - 1; ++i){
			ret->add(*(isLastItemInParent[i] ? stob::parse(DEmpty) : stob::parse(DLine)));
		}
		
		{
			auto widget = std::dynamic_pointer_cast<morda::Pile>(morda::Morda::inst().inflater.inflate(*stob::parse(isLastItemInParent.back() ? DLineEnd : DLineMiddle)));
			ASSERT(widget)
			
			if(n->child()){
				auto w = morda::Morda::inst().inflater.inflate(*stob::parse(DPlusMinus));

				auto plusminus = w->findByNameAs<morda::Image>("plusminus");
				ASSERT(plusminus)
				plusminus->setImage(
						isCollapsed ?
								morda::Morda::inst().resMan.load<morda::ResImage>("morda_img_treeview_plus") :
								morda::Morda::inst().resMan.load<morda::ResImage>("morda_img_treeview_minus")
					);

				auto plusminusMouseProxy = w->findByNameAs<morda::MouseProxy>("plusminus_mouseproxy");
				ASSERT(plusminusMouseProxy)
				plusminusMouseProxy->mouseButton = [this, path, isCollapsed](morda::Widget& widget, bool isDown, const morda::Vec2r& pos, morda::MouseButton_e button, unsigned pointerId) -> bool{
					if(button != morda::MouseButton_e::LEFT){
						return false;
					}
					if(!isDown){
						return false;
					}

					if(isCollapsed){
						this->uncollapse(path);
					}else{
						this->collapse(path);
					}

					TRACE_ALWAYS(<< "plusminus clicked:")
					for(auto i = path.begin(); i != path.end(); ++i){
						TRACE_ALWAYS(<< " " << (*i))
					}
					TRACE_ALWAYS(<< std::endl)

					return true;
				};
				widget->add(w);
			}
			ret->add(widget);
		}
		
		{
			auto v = morda::Morda::inst().inflater.inflate(*stob::parse(
					R"qwertyuiop(
							Pile{
								Color{
									name{selection}
									layout{dx{max}dy{max}}
									color{@{morda_color_highlight}}
									visible{false}
								}
								Text{
									name{value}
								}
								MouseProxy{
									name{mouse_proxy}
									layout{dx{max}dy{max}}
								}
							}
						)qwertyuiop"
				));
			
			{
				auto value = v->findByNameAs<morda::Text>("value");
				ASSERT(value)
				value->setText(n->value());
			}
			{
				auto colorLabel = v->findByNameAs<morda::Color>("selection");
				
				colorLabel->setVisible(this->selectedItem == path);
				
				auto mp = v->findByNameAs<morda::MouseProxy>("mouse_proxy");
				ASSERT(mp)
				mp->mouseButton = [this, path](morda::Widget&, bool isDown, const morda::Vec2r&, morda::MouseButton_e button, unsigned pointerId) -> bool{
					if(!isDown || button != morda::MouseButton_e::LEFT){
						return false;
					}
					
					this->selectedItem = path;
					this->notifyItemChanged();
					//TODO:
					
					return true;
				};
			}

			ret->add(v);
		}
		
		{
			auto b = std::dynamic_pointer_cast<morda::PushButton>(morda::Morda::inst().inflater.inflate(*stob::parse(
					R"qwertyuiop(
							PushButton{
								Color{
									color{0xff0000ff}
									layout{dx{2mm}dy{0.5mm}}
								}
							}
						)qwertyuiop"
				)));
			b->clicked = [this, path, n, parent](morda::PushButton& button){
				ASSERT(parent)
				parent->removeChild(n);
				this->notifyItemRemoved(path);
			};
			ret->add(b);
		}
		
		return ret;
	}
	
	size_t count(const std::vector<size_t>& path) const noexcept override{
		auto n = this->root.get();
		
		for(auto i = path.begin(); i != path.end(); ++i){
			n = n->child(*i).node();
		}
		
		return n->count();
	}

};



class Application : public mordavokne::App{
	static mordavokne::App::WindowParams GetWindowParams()noexcept{
		mordavokne::App::WindowParams wp(kolme::Vec2ui(1024, 800));
		
		return wp;
	}
public:
	Application() :
			App(GetWindowParams())
	{
		morda::Morda::inst().initStandardWidgets(*this->getResFile());
		
		morda::Morda::inst().resMan.mountResPack(*this->getResFile("res/"));
//		this->ResMan().MountResPack(morda::ZipFile::New(papki::FSFile::New("res.zip")));
		
		morda::Morda::inst().inflater.addWidget<SimpleWidget>("U_SimpleWidget");
		morda::Morda::inst().inflater.addWidget<CubeWidget>("CubeWidget");

		std::shared_ptr<morda::Widget> c = morda::Morda::inst().inflater.inflate(
				*this->getResFile("res/test.gui")
			);
		morda::Morda::inst().setRootWidget(c);
		
		std::dynamic_pointer_cast<morda::KeyProxy>(c)->key = [this](bool isDown, morda::Key_e keyCode) -> bool{
			if(isDown){
				if(keyCode == morda::Key_e::ESCAPE){
					this->quit();
				}
			}
			return false;
		};

//		morda::ZipFile zf(papki::FSFile::New("res.zip"), "test.gui.stob");
//		std::shared_ptr<morda::Widget> c = morda::Morda::inst().inflater().Inflate(zf);
		
		
		std::dynamic_pointer_cast<morda::PushButton>(c->findByName("show_VK_button"))->clicked = [this](morda::PushButton&){
			this->showVirtualKeyboard();
		};
		
		std::dynamic_pointer_cast<morda::PushButton>(c->findByName("push_button_in_scroll_container"))->clicked = [this](morda::PushButton&){
			morda::Morda::inst().postToUiThread_ts(
					[](){
						TRACE_ALWAYS(<< "Print from UI thread!!!!!!!!" << std::endl)
					}
				);
		};
		
		std::dynamic_pointer_cast<CubeWidget>(c->findByName("cube_widget"))->startUpdating(0);
		
		//ScrollArea
		{
			auto scrollArea = c->findByNameAs<morda::ScrollArea>("scroll_area");
			auto sa = utki::makeWeak(scrollArea);
			
			auto vertSlider = c->findByNameAs<morda::ScrollBar>("scroll_area_vertical_slider");
			auto vs = utki::makeWeak(vertSlider);
			
			auto horiSlider = c->findByNameAs<morda::ScrollBar>("scroll_area_horizontal_slider");
			auto hs = utki::makeWeak(horiSlider);
			
			auto resizeProxy = c->findByNameAs<morda::ResizeProxy>("scroll_area_resize_proxy");
			auto rp = utki::makeWeak(resizeProxy);
			
			resizeProxy->resized = [vs, hs, sa](const morda::Vec2r& newSize){
				auto sc = sa.lock();
				if(!sc){
					return;
				}
				
				if(auto v = vs.lock()){
					v->setFraction(sc->scrollFactor().y);
				}
				if(auto h = hs.lock()){
					h->setFraction(sc->scrollFactor().x);
				}
			};
			
			
			vertSlider->fractionChange = [sa](morda::FractionWidget& slider){
				if(auto s = sa.lock()){
					auto sf = s->scrollFactor();
					sf.y = slider.fraction();
					s->setScrollPosAsFactor(sf);
				}
			};
			
			horiSlider->fractionChange = [sa](morda::FractionWidget& slider){
				if(auto s = sa.lock()){
					auto sf = s->scrollFactor();
					sf.x = slider.fraction();
					s->setScrollPosAsFactor(sf);
				}
			};
		}
		
		//VerticalList
		{
			auto verticalList = c->findByNameAs<morda::VList>("vertical_list");
			std::weak_ptr<morda::VList> vl = verticalList;
			
			auto verticalSlider = c->findByNameAs<morda::VScrollBar>("vertical_list_slider");
			std::weak_ptr<morda::VScrollBar> vs = verticalSlider;
			
			verticalSlider->fractionChange = [vl](morda::FractionWidget& slider){
				if(auto l = vl.lock()){
					l->setScrollPosAsFactor(slider.fraction());
				}
			};
			
			auto resizeProxy = c->findByNameAs<morda::ResizeProxy>("vertical_list_resize_proxy");
			ASSERT(resizeProxy)
			
			resizeProxy->resized = [vs, vl](const morda::Vec2r& newSize){
				auto l = vl.lock();
				if(!l){
					return;
				}
				if(auto s = vs.lock()){
//					auto f = std::move(s->factorChange);
					s->setFraction(l->scrollFactor());
//					s->factorChange = std::move(f);
				}
			};
		}


		//fullscreen
		{
			auto b = c->findByNameAs<morda::PushButton>("fullscreen_button");
			b->clicked = [this](morda::PushButton&) {
				this->setFullscreen(!this->isFullscreen());
			};
		}
		
		//mouse cursor
		{
			auto b = c->findByNameAs<morda::PushButton>("showhide_mousecursor_button");
			bool visible = false;
			this->setMouseCursorVisible(visible);
			b->clicked = [this, visible](morda::PushButton&) mutable{
				visible = !visible;
				mordavokne::App::inst().setMouseCursorVisible(visible);
			};
		}
		TRACE(<< "Application constructor exit" << std::endl)
	}
};



std::unique_ptr<mordavokne::App> mordavokne::createApp(int argc, const char** argv, const utki::Buf<std::uint8_t> savedState){
	return utki::makeUnique<Application>();
}
