import PotatoTaskSystem;

using namespace Potato::Task;


int main()
{
	{
		auto Ptr = TaskContext::Create();
		auto Lambda = Ptr->CreateLambdaTask({}, [](TaskContext::Ptr Context){
		
			volatile int o = 0;
		});
		Lambda->Commit();
		std::this_thread::sleep_for(std::chrono::seconds{10});
	}
}