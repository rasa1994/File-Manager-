module;

export module CopyFinderModule;

export import <iostream>;
export import <filesystem>;
import <vector>;
import <concepts>;
export import <unordered_map>;
export import <set>;
export import <fstream>;
import <algorithm>;
import <numeric>;
import <fstream>;
import <list>;
import <ranges>;
import <thread>;

struct FileInfo
{
	long long fileSize;
	std::string filePath;
	constexpr bool operator < (const FileInfo& other) const
	{
		if (other.fileSize == fileSize)
			return filePath == other.filePath;

		return fileSize < other.fileSize;
	}

	constexpr bool operator == (const FileInfo& other) const
	{
		if (other.fileSize == fileSize)
			return filePath == other.filePath;

		return false;
	}
};

// extension key - FileInfo data
std::unordered_map<std::string, std::multiset<FileInfo>> files;
std::vector<std::string> copiesForDelete;
std::vector<std::thread> threadsForDuplicates;


std::string GetFileExtension(const std::string& file);
bool CompareFile(const std::string& file1, const std::string& file2);

void ThreadHelper(const std::string& extension, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>>& copiesOfElems)
{
	const auto& elements = files[extension];
	std::list<FileInfo> filesWithInfos;

	for (const auto& elem : elements)
		filesWithInfos.push_back(elem);
	
	copiesOfElems[extension] = std::unordered_map<std::string, std::vector<std::string>>();
	for (auto listIterator = filesWithInfos.begin(); listIterator != filesWithInfos.end(); ++listIterator)
	{
		copiesOfElems[extension][listIterator->filePath] = std::vector<std::string>();
		for (auto innerIt = listIterator; innerIt != filesWithInfos.end();)
		{
			if (*listIterator == *innerIt)
			{
				++innerIt;
				continue;
			}

			if (listIterator->fileSize != innerIt->fileSize)
				break;

			if (CompareFile(listIterator->filePath, innerIt->filePath))
			{
				copiesOfElems[extension][listIterator->filePath].push_back(innerIt->filePath);
				innerIt = filesWithInfos.erase(innerIt);
			}
			else
			{
				++innerIt;
			}

		}

	}
}

using namespace std::filesystem;

template <typename T>
concept DirectoryEntry = std::same_as<T, directory_entry>;

template <typename T>
requires DirectoryEntry<T>
void AddFileToListOfFiles(const T& fileIt)
{
	if (!fileIt.is_directory())
	{
		FileInfo info;
		info.filePath = fileIt.path().string();
		info.fileSize = fileIt.file_size();
		auto extension = GetFileExtension(info.filePath);
		if (files.contains(extension))
		{
			files[extension].insert(info);
		}
		else
		{
			std::multiset<FileInfo> newFilesSet;
			newFilesSet.insert(info);
			files[extension] = newFilesSet;
		}
	}
}


export
{
	std::string GetFileExtension(const std::string& file)
	{
		return path(file).extension().string();
	}

	bool DeleteFile(const std::string& file)
	{
		if (exists(file))
		{
			remove(file.c_str());
			return true;
		}
		return false;
	}

	bool CompareFile(const std::string& file1, const std::string& file2)
	{
		std::ifstream f1(file1, std::ifstream::binary | std::ifstream::ate);
		std::ifstream f2(file2, std::ifstream::binary | std::ifstream::ate);
		if (f1 && f2)
		{
			if (f1.tellg() != f2.tellg())
			{
				return false;
			}
			f1.seekg(0);
			f2.seekg(0);

			return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
				std::istreambuf_iterator<char>(),
				std::istreambuf_iterator<char>(f2.rdbuf()));
		}
		return false;
	}


	void PrintFiles()
	{
		std::vector<std::string> filesElements;
		for (const auto& extension : files)
		{
			std::cout << "With extension: " << extension.first << std::endl;
			for (const auto& fileInfos : extension.second)
			{
				filesElements.push_back(fileInfos.filePath);
				std::cout << "File: " << fileInfos.filePath << std::endl;
				std::cout << "Size: " << fileInfos.fileSize << std::endl;
			}
		}
		std::cout << std::boolalpha;
		std::cout << CompareFile(filesElements[0], filesElements[1]);
	}


	void LoadAllFiles(bool recursive, const std::string_view& path = ".")
	{
		files.clear();

		if (recursive)
		{
			for (const auto& fileIt : recursive_directory_iterator(path))
			{
				AddFileToListOfFiles(fileIt);
			}
		}
		else
		{
			for (const auto& fileIt : directory_iterator(path))
			{
				AddFileToListOfFiles(fileIt);
			}
		}
	}

	void ListAllFiles(bool recursive = false, const std::string& path = ".")
	{ 
		if (files.empty())
			LoadAllFiles(recursive, path);
	}

	void ListAllDuplicates(const std::unordered_map<std::string, bool>& extensions, bool recursive = false, const std::string& path = ".")
	{
		threadsForDuplicates.clear();
		copiesForDelete.clear();
		LoadAllFiles(recursive, path);

		// too complex
		std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> copiesOfElems;

		std::erase_if(files, [extensions](const auto& fileValue) 
			{
				return !extensions.contains(fileValue.first);
			});

		
		for (const auto& elems : std::ranges::views::keys(files))
		{
			threadsForDuplicates.emplace_back(&ThreadHelper, elems, std::ref(copiesOfElems));
		}

		for (auto& thread : threadsForDuplicates)
			thread.join();

		for (const auto& file : copiesOfElems)
		{
			std::ofstream out;
			std::string outputSource = path;
			outputSource.append("\\");
			outputSource.append(file.first);
			outputSource.append("_copies.txt");
			out.open(outputSource.c_str());

			auto mapOfFilesOfExtension = file.second;

			for (const auto& copies : mapOfFilesOfExtension)
			{
				if (!copies.second.empty())
				{
					out << "File: " << copies.first << " has copies:" << std::endl;
					unsigned long long copyNumber = 1;
					for (const auto& copy : copies.second)
					{
						copiesForDelete.push_back(copy);
						out << "\t" << std::to_string(copyNumber++) << ": " << copy << std::endl;
					}
				}
			}


			out.close();
		}

	}

	void DeleteAllCopies()
	{
		unsigned long deleted = 0;
		for (const auto& path : copiesForDelete)
			if (DeleteFile(path))
				deleted++;

		std::cout << "Number of files deleted: " << deleted << std::endl;
	}

	void RenameFiles(const std::string& path, const std::string& prefix, const std::string& extension = "", const int nthToBeRemoved = -1)
	{
		unsigned long fileNumber = 1;
		unsigned long fileCounter = 1;
		for (const auto& pathIt : directory_iterator(path))
		{
			if (!std::filesystem::is_directory(pathIt))
			{
				auto extensionInner = GetFileExtension(pathIt.path().string());
				if (!extension.empty())
				{
					if (extensionInner != extension)
						continue;
				}
				fileCounter++;
				if (nthToBeRemoved > 0 && fileCounter % nthToBeRemoved == 0)
				{
					remove(pathIt.path().c_str());
					continue;
				}

				auto newName = path;
				newName.append("\\");
				newName.append(prefix);
				newName.append(std::to_string(fileNumber++));
				newName.append(extensionInner);
				rename(pathIt.path().c_str(), newName.c_str());
			}
		}

	}

	void SearchFiles(const std::string& path, const std::string& prefix, const std::string& suffix, const std::string& infix)
	{
		for (auto it : recursive_directory_iterator(path))
		{
			auto fileName = it.path().filename().string();

			for (auto& ch : fileName)
				ch = std::tolower(ch);

			bool ok = true;
			if (!prefix.empty())
			{
				std::string lowerPrefix;

				for (const auto& a : prefix)
					lowerPrefix.push_back(std::tolower(a));

				ok &= fileName.find(lowerPrefix) == 0;
				if (!ok)
					continue;
			}
			if (!infix.empty())
			{
				std::string lowerInfix;

				for (const auto& a : infix)
					lowerInfix.push_back(std::tolower(a));

				ok &= fileName.find(lowerInfix) != std::string::npos;
				if (!ok)
					continue;
			}
			if (!suffix.empty())
			{
				std::string lowerSuffix;

				for (const auto& a : lowerSuffix)
					lowerSuffix.push_back(std::tolower(a));

				auto pos = fileName.find(lowerSuffix);
				ok &= (pos != std::string::npos && pos == (fileName.size() - lowerSuffix.size()));
				if (!ok)
					continue;
			}

			if (ok)
				std::cout << "Found file on path: " << it.path().string() << std::endl;
		}
	}

	void StartProgram(char** args)
	{
		std::string currentPath = args[0];
		auto pos = currentPath.find_last_of('\\');
		currentPath = currentPath.substr(0, pos);
		std::unordered_map<std::string, bool> extensions;
		while (true)
		{
			std::cout << "Current path is: " << std::endl << currentPath << std::endl;
			std::cout << "Press 1. to change current path" << std::endl;
			std::cout << "Press 2. to list duplicates" << std::endl;
			std::cout << "Press 3. To rename all files in folder" << std::endl;
			std::cout << "Press 4. to search for file" << std::endl;

			int in;
			std::cin >> in;

			switch (in)
			{
			case 1:
			{

				bool ok = true;
				do
				{
					std::cout << "Type in new FULL path" << std::endl;
					std::string newPath;
					std::cin >> newPath;

					if (!std::filesystem::exists(newPath))
					{
						std::cout << "Please type in ok path!" << std::endl;
						ok = false;
					}
					else
					{
						currentPath = newPath;
						ok = true;
					}
				} while (!ok);
			}; break;
			case 2:
			{
				bool ok = true;
				do
				{
					std::cout << "Press 1. to add extension to be checked!" << std::endl;
					std::cout << "Press 2. to list duplicates in current folder" << std::endl;
					std::cout << "Press 3. to list duplicates recursively" << std::endl;
					std::cout << "Press 4. to clear extensions" << std::endl;
					std::cout << "Press any other for back" << std::endl;
					if (!extensions.empty())
					{
						std::cout << "Extensions to be checked: " << std::endl;
						for (const auto& extension : std::ranges::views::keys(extensions))
							std::cout << "\t" << extension << std::endl;
					}
					int value;
					std::cin >> value;

					switch (value)
					{
					case 1:
					{
						std::cout << "Insert new extension" << std::endl;
						std::string inInner;
						std::cin >> inInner;
						extensions[inInner] = true;
					} break;
					case 2:
					{
						ListAllDuplicates(extensions, false, currentPath);
						std::cout << "All duplicates listed in " << currentPath << " in files arranged by extension " << std::endl;
						std::cout << "Press 1. to delete those duplicates" << std::endl;
						std::cout << "Press any for back." << std::endl;
						int inDelete;
						std::cin >> inDelete;
						switch (inDelete)
						{
						case 1: { DeleteAllCopies(); };
						default:
							ok = false;
							break;
						}
					} break;
					case 3:
					{
						ListAllDuplicates(extensions, true, currentPath);
						std::cout << "All duplicates listed in " << currentPath << " in files arranged by extension " << std::endl;
						std::cout << "Press 1. to delete those duplicates" << std::endl;
						std::cout << "Press any for back." << std::endl;
						int inDelete;
						std::cin >> inDelete;
						switch (inDelete)
						{
						case 1: { DeleteAllCopies(); };
						default:
							ok = false;
							break;
						}
					} break;
					case 4: {

						extensions.clear();

					} break;
					default:
						ok = false;
						break;
					}


				} while (ok);
			} break;
			case 3:
			{
				std::string prefix;
				std::string extension;
				int nthToBeRemoved = -1;
				bool ok = true;
				do
				{
					std::cout << "Press 1. to change extension of files to be renamed" << std::endl;
					std::cout << "Press 2. to enter new prefix name for files" << std::endl;
					std::cout << "Press 3. to rename with current settings" << std::endl;
					std::cout << "Press 4. to set n-th file to be renamed" << std::endl;
					int input2;
					std::cin >> input2;
					switch (input2)
					{
					case 1:
					{
						std::cout << "Input extension" << std::endl;
						std::cin >> extension;
					}break;
					case 2:
					{
						std::cout << "Input prefix" << std::endl;
						std::cin >> prefix;
					}break;
					case 3:
					{
						ok = false;
						RenameFiles(currentPath, prefix, extension, nthToBeRemoved);
					}break;
					case 4:
					{
						std::cout << "Input which nth file will be removed" << std::endl;
						std::cin >> nthToBeRemoved;
					}break;
					default: break;
					}
				} while (ok);
			}break;
			case 4:
			{
				bool ok = true;
				std::string prefix, suffix, infix;
				do
				{
					std::cout << "Press 1: to input prefix" << std::endl;
					std::cout << "Press 2: to input suffix" << std::endl;
					std::cout << "Press 3: to input infix" << std::endl;
					std::cout << "Press 4: to search" << std::endl;


					if (!prefix.empty() || !infix.empty() || !infix.empty())
					{
						std::cout << "Files to be searched are like " << prefix << "__" << infix << "__" << suffix << std::endl;
					}
					int inInner;
					std::cin >> inInner;
					switch (inInner)
					{
					case 1: {

						std::cout << "Type new prefix" << std::endl;
						std::cin >> prefix;

					} break;
					case 2: {


						std::cout << "Type new sufix" << std::endl;
						std::cin >> suffix;

					} break;
					case 3: {


						std::cout << "Type new infix" << std::endl;
						std::cin >> infix;

					} break;
					case 4:
					{
						SearchFiles(currentPath, prefix, suffix, infix);
						ok = false;
					}
					default:
						break;
					}
				} while (ok);

			}break;



			default: std::cout << "WRONG INPUT!" << std::endl;
			}
		}
	}
}
