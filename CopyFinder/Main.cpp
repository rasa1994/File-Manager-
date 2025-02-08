import CopyFinderModule;

int ResultRunTests();

auto main(int nargs, char**args) -> int
{
	if (!ResultRunTests())
	{
		std::cout << "Tests failed!" << std::endl;
		return 1;
	}
	//StartProgram(args);
	return 0;
}