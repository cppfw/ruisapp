// TODO: move to glue.cxx
class ruisapp::application::platform_glue : public utki::destructable
{
public:
	nitki::queue ui_queue;

	std::atomic_bool quit_flag = false;
};
