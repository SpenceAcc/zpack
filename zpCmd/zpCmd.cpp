// ptest.cpp : Defines the entry point for the console application.
//

//#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include "zpack.h"
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include "zpexplorer.h"
//#include "fileenum.h"

using namespace std;

string getWord(const string& input, size_t& pos)
{
	size_t start = input.find_first_not_of(' ', pos);
	if (start == string::npos)
	{
		start = pos;
	}
	pos = input.find_first_of(' ', start);
	if (pos == string::npos)
	{
		pos = input.length();
	}
	return input.substr(start, pos - start);
}

bool zpcallback(const string& path, size_t fileIndex, size_t totalFileCount)
{
	cout << path << endl;
	return true;
}

typedef bool (*CommandProc)(const string& param0, const string& param1);

map<string, CommandProc> g_commandHandlers;

#define CMD_PROC(cmd) bool cmd##_proc(const string& param0, const string& param1)
#define REGISTER_CMD(cmd) g_commandHandlers[#cmd] = &cmd##_proc;

string g_packName;
ZpExplorer g_explorer;

CMD_PROC(exit)
{
	exit(0);
	return true;
}

CMD_PROC(open)
{
	if (g_explorer.open(param0))
	{
		g_packName = param0;
		return true;
	}
	return false;
}

CMD_PROC(create)
{
	if (g_explorer.create(param0, param1))
	{
		g_packName = param0;
		return true;
	}
	return false;
}

CMD_PROC(close)
{
	g_packName.clear();
	g_explorer.close();
	return true;
}

CMD_PROC(list)
{
	zp::IPackage* pack = g_explorer.getPack();
	if (pack == NULL)
	{
		return false;
	}
	size_t count = pack->getFileCount();
	for (unsigned long i = 0; i < count; ++i)
	{
		char filename[256];
		pack->getFilenameByIndex(filename, sizeof(filename), i);
		zp::IFile* file = pack->openFile(filename);
		if (file != NULL)
		{
			cout << filename << "[" << file->getSize() << "]" << endl;
			pack->closeFile(file);
		}
		else
		{
			cout << filename << endl;
		}
	}
	return true;
}

CMD_PROC(dir)
{
	const ZpNode* node = g_explorer.getNode();
	for (list<ZpNode>::const_iterator iter = node->children.begin();
		iter != node->children.end();
		++iter)
	{
		if (iter->isDirectory)
		{
			cout << "  <" << iter->name << ">" << endl; 
		}
		else
		{
			cout << "  " << iter->name << endl; 
		}
	}
	return true;
}

CMD_PROC(cd)
{
	if (param0 == "..")
	{
		g_explorer.exit();
	}
	else if (param0 == "/")
	{
		g_explorer.enterRoot();
	}
	else
	{
		g_explorer.enter(param0);
	}
	return true;
}

CMD_PROC(add)
{
	return g_explorer.add(param0);
}

CMD_PROC(del)
{
	return g_explorer.remove(param0);
}

CMD_PROC(extract)
{
	return g_explorer.extract(param0, param1);
}

CMD_PROC(fragment)
{
	zp::IPackage* pack = g_explorer.getPack();
	if (pack == NULL)
	{
		return false;
	}
	unsigned __int64 toMove;
	unsigned __int64 fragSize = pack->countFragmentSize(toMove);
	cout << "fragment:" << fragSize << " bytes, " << toMove << " bytes need moving" << endl;
	return true;
}

CMD_PROC(defrag)
{
	zp::IPackage* pack = g_explorer.getPack();
	if (pack == NULL)
	{
		return false;
	}
	return (pack != NULL && pack->defrag());
}

CMD_PROC(help)
{
#define HELP_ITEM(cmd, explain) cout << cmd << endl << "    "explain << endl;
	HELP_ITEM("create [package path] [initial dir path]", "create a package from scratch");
	HELP_ITEM("open [package path]", "open an existing package");
	HELP_ITEM("close", "close current package");
	HELP_ITEM("list", "show all files in current package");
	HELP_ITEM("cd [path]", "enter specified sub-directory of package");
	HELP_ITEM("dir", "show all files and directories of current directory of package");
	HELP_ITEM("add [file/directory path]", "add a disk file or directory to current directory of package");
	HELP_ITEM("del [file/directory", "delete a file or sub-directory from current directory of package");
	HELP_ITEM("extract [file/directory path", "extrace a file or sub-directory to disk from current directory of package");
	HELP_ITEM("fragment", "calculate fragment bytes and how many bytes to move to defrag");
	HELP_ITEM("defrag", "compact file, remove all fragments");
	HELP_ITEM("exit", "exit program");
	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	g_explorer.setCallback(zpcallback);

	REGISTER_CMD(exit);
	REGISTER_CMD(create);
	REGISTER_CMD(open);
	REGISTER_CMD(add);
	REGISTER_CMD(del);
	REGISTER_CMD(extract);
	REGISTER_CMD(close);
	REGISTER_CMD(list);
	REGISTER_CMD(dir);
	REGISTER_CMD(cd);
	REGISTER_CMD(fragment);
	REGISTER_CMD(defrag);
	REGISTER_CMD(help);

	cout << "please type <help> to checkout commands available." << endl;
	while (true)
	{
		if (!g_packName.empty())
		{
			cout << g_packName;
			if (g_explorer.isOpen())
			{
				cout << "/" << g_explorer.getPath();
			}
			cout << ">";
		}
		else
		{
			cout << "zp>";
		}
		string input, command, param0, param1;
		getline(cin, input);

		size_t pos = 0;

		istringstream iss(input, istringstream::in);
		iss >> command;
		iss >> param0;
		iss >> param1;
		map<string, CommandProc>::iterator found = g_commandHandlers.find(command);
		if (found == g_commandHandlers.end())
		{
			cout << "<" << command << "> command not found." << endl;
		}
		else if (!found->second(param0, param1))
		{
			cout << "<" << command << "> failed." << endl;
		}
	}
	return 0;
}
