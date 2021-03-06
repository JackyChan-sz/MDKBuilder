// MDKBuild.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <errno.h>
#include <io.h>
#include <vector>
#include <io.h>
#include <process.h>
#include <direct.h>
#include "..\\getopt\\getopt.h"
#include "..\\tinyxml2-6.2.0\\tinyxml2.h"

using namespace std;
using namespace tinyxml2;

const int BUFF_SIZE = 1024;

char g_szCurrPath[MAX_PATH] = { '\0' };

const char XML_FILE[] = "\\MDKBuild.xml";

typedef struct tagOBJECT
{
	char szBuffer[BUFF_SIZE];
	char szDstPath[BUFF_SIZE];
	tagOBJECT()
	{
		memset(szBuffer, 0, sizeof(szBuffer));
		memset(szDstPath, 0, sizeof(szDstPath));
	}
}OBJECT;

typedef struct tagVERSION
{
	char szBuffer[BUFF_SIZE];
	char szIgnore[BUFF_SIZE];
	tagVERSION() 
	{ 
		memset(szBuffer, 0, sizeof(szBuffer));
		memset(szIgnore, 0, sizeof(szIgnore));
	}
}VERSION;

typedef struct tagSourceInfo
{
	char szSectionName[BUFF_SIZE];

	char szSourceUrl[BUFF_SIZE];
	char szRevisionUrl[BUFF_SIZE];
	char szRevisionString[BUFF_SIZE];
	char szVerionFile[BUFF_SIZE];
	vector<VERSION> vecVersionString;
	char szKeilPath[BUFF_SIZE];
	char szProject[BUFF_SIZE];
	char szPrevOption[BUFF_SIZE];
	char szPostOption[BUFF_SIZE];
	//char szOutPath[BUFF_SIZE];
	vector<OBJECT> vecObjName;

	DWORD dwRevision;
	tagSourceInfo()
	{
		memset(szSectionName, 0, sizeof(szSectionName));

		memset(szSourceUrl, 0, sizeof(szSourceUrl));
		memset(szRevisionUrl, 0, sizeof(szRevisionUrl));
		memset(szRevisionString, 0, sizeof(szRevisionString));
		memset(szVerionFile, 0, sizeof(szVerionFile));
		vecVersionString.clear();
		memset(szKeilPath, 0, sizeof(szKeilPath));
		memset(szProject, 0, sizeof(szProject));
		memset(szPrevOption, 0, sizeof(szPrevOption));
		memset(szPostOption, 0, sizeof(szPostOption));
		//memset(szOutPath, 0, sizeof(szOutPath));
		vecObjName.clear();

		dwRevision = (DWORD)(-1);
	}
}SourceInfo;

vector<SourceInfo> g_vecSrcCode;

char g_szDebug[BUFF_SIZE] = { '\0' };
char g_szCommitUrl[BUFF_SIZE] = { '\0' };
char g_szPrefix[BUFF_SIZE] = { '\0' };
char g_szSuffix[BUFF_SIZE] = { '\0' };
char g_szLabel[BUFF_SIZE] = { '\0' };

const char DST_PATH[] = "\\working";
const char COMMIT_PATH[] = "\\commitfiles";
const char OUT_SRC[] = "\\01-src";
const char OUT_BIN[] = "\\02-bin";
const char OUT_DOC[] = "\\03-doc";

bool g_bLeftPadWithZero = false;

BOOL CreateWorkingPath(void)
{
	int status;

	// 工作路径
	char szDstPath[BUFF_SIZE] = { 0 };

	// 工作路径
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s", g_szCurrPath, DST_PATH);

	// 删除并重新创建工作路径
	char szCommand[BUFF_SIZE] = { '\0' };
	sprintf_s(szCommand, BUFF_SIZE, "attrib -s -r -h \"%s\"  /S /D /L", szDstPath);
	status = system(szCommand);
	sprintf_s(szCommand, BUFF_SIZE, "rd /s /q \"%s\"", szDstPath);
	status = system(szCommand);
	sprintf_s(szCommand, BUFF_SIZE, "mkdir \"%s\"", szDstPath);
	status = system(szCommand);

	// 在工作路径下创建提交文件路径
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s%s", g_szCurrPath, DST_PATH, COMMIT_PATH);
	sprintf_s(szCommand, BUFF_SIZE, "mkdir \"%s\"", szDstPath);
	system(szCommand);

	// 在提交文件路径创建源代码路径“01-src”
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s%s%s", g_szCurrPath, DST_PATH, COMMIT_PATH, OUT_SRC);
	sprintf_s(szCommand, BUFF_SIZE, "mkdir \"%s\"", szDstPath);
	system(szCommand);

	// 在提交文件路径创建目标代码路径“02-bin”
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s%s%s", g_szCurrPath, DST_PATH, COMMIT_PATH, OUT_BIN);
	sprintf_s(szCommand, BUFF_SIZE, "mkdir \"%s\"", szDstPath);
	system(szCommand);

	// 在提交文件路径创建文档路径“03-doc”
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s%s%s", g_szCurrPath, DST_PATH, COMMIT_PATH, OUT_DOC);
	sprintf_s(szCommand, BUFF_SIZE, "mkdir \"%s\"", szDstPath);
	system(szCommand);

	return TRUE;
}

BOOL ReadConfiguration(void)
{
	DWORD dwRet = 0;

	// #00.获取当前路径
	dwRet = GetCurrentDirectoryA(MAX_PATH, g_szCurrPath);
	if (0 == dwRet || dwRet > MAX_PATH)
	{
		printf("\nGet current directory error!\n\n");
		system("pause");
		return FALSE;
	}

	// #00.配置文件全名（含路径）
	char szFileName[MAX_PATH];
	sprintf_s(szFileName, MAX_PATH, "%s%s", g_szCurrPath, XML_FILE);
	tinyxml2::XMLDocument doc;
	if (XML_SUCCESS != doc.LoadFile(szFileName))
	{
		printf("\nXML file has some error!\n\n");
		system("pause");
		return FALSE;
	}
	XMLElement* build = doc.FirstChildElement("mdkbuild");
	if (NULL == build)
	{
		printf("\nXML file has not root element!\n\n");
		system("pause");
		return FALSE;
	}

	// Debug 
	XMLElement* debug = build->FirstChildElement("debug");
	if (NULL != debug)
	{
		strcpy_s(g_szDebug, sizeof(g_szDebug), debug->Attribute("name"));
	}

	// #01.读取目标文件提交链接
	XMLElement* commit = build->FirstChildElement("commit");
	if ((NULL == commit) && (0 == strnlen_s(commit->GetText(), BUFF_SIZE)))
	{
		printf("\nGet commit url error!\n\n");
		system("pause");
		return FALSE;
	}
	else
	{
		strcpy_s(g_szCommitUrl, BUFF_SIZE, commit->GetText());
	}

	// #02.读取标签/基线名称
	XMLElement* baseline = build->FirstChildElement("baseline");
	if (NULL == baseline)
	{
		printf("\nGet baseline error!\n\n");
		system("pause");
		return FALSE;
	}
	else
	{
		// #02.读取前缀字符串
		XMLElement* prefix = baseline->FirstChildElement("prefix");
		if (NULL != prefix)
		{
			strcpy_s(g_szPrefix, BUFF_SIZE, prefix->GetText());
		}

		// #02.读取后缀字符串
		XMLElement* suffix = baseline->FirstChildElement("suffix");
		if (NULL != suffix)
		{
			strcpy_s(g_szSuffix, BUFF_SIZE, suffix->GetText());
		}

		// #02.读取Label字符串
		XMLElement* label = baseline->FirstChildElement("label");
		if (NULL != label)
		{
			strcpy_s(g_szLabel, BUFF_SIZE, label->GetText());
		}
	}

	/* #03.读取所有的源代码信息 */
	g_vecSrcCode.clear();
	XMLElement* source = build->FirstChildElement("source");
	while (NULL != source)
	{
		// #03.00.段名
		SourceInfo si;
		strcpy_s(si.szSectionName, sizeof(si.szSectionName), source->Attribute("name"));

		// #03.01.读取源代码URL
		XMLElement* source_url = source->FirstChildElement("source_url");
		if (NULL == source_url)
		{
			printf("\nGet %s source code url error!\n\n", si.szSectionName);
			system("pause");
			return FALSE;
		}
		else
		{
			strcpy_s(si.szSourceUrl, BUFF_SIZE, source_url->GetText());
		}

		// #03.01.读取源代码获取版本号的URL
		XMLElement* revision_url = source->FirstChildElement("revision_url");
		if (NULL != revision_url)
		{
			strcpy_s(si.szRevisionUrl, BUFF_SIZE, revision_url->GetText());
		}

		// #03.02.读取指定的export源代码的revision号
		XMLElement* revision = source->FirstChildElement("revision");
		if (NULL == revision)
		{
			printf("\nGet %s source code revision error!\n\n", si.szSectionName);
			system("pause");
			return FALSE;
		}
		else
		{
			strcpy_s(si.szRevisionString, BUFF_SIZE, revision->GetText());
		}

		// #03.03.读取version.c的文件相对路径
		XMLElement* version_c = source->FirstChildElement("version.c");
		if (NULL == version_c)
		{
			printf("\nGet %s's path of version.c error!\n\n", si.szSectionName);
			system("pause");
			return FALSE;
		}
		else
		{
			strcpy_s(si.szVerionFile, BUFF_SIZE, version_c->GetText());
		}

		// #03.03.读取version.c的文件的写入内容模板
		XMLElement* format = source->FirstChildElement("format");
		while (NULL != format)
		{
			VERSION ver_str;
			strcpy_s(ver_str.szBuffer, BUFF_SIZE, format->Attribute("name"));
			if (format->GetText())
			{
				strcpy_s(ver_str.szIgnore, BUFF_SIZE, format->GetText());
			}

			si.vecVersionString.push_back(ver_str);
			format = format->NextSiblingElement("format");
		}

		// #03.03.version.c的文件的写入内容模板，至少必须有一个
		if (0 == si.vecVersionString.size())
		{
			printf("\nGet %s's template of version.c error!\n\n", si.szSectionName);
			system("pause");
			return FALSE;
		}

		// #03.04.读取Keil MDK编译器的路径
		XMLElement* compiler = source->FirstChildElement("compiler");
		if (NULL == compiler)
		{
			printf("\nGet %s compiler error!\n\n", si.szSectionName);
			system("pause");
			return FALSE;
		}
		else
		{
			strcpy_s(si.szKeilPath, BUFF_SIZE, compiler->GetText());
		}

		// #03.05.读取源代码的项目工程文件
		XMLElement* project = source->FirstChildElement("project");
		if (NULL == project)
		{
			printf("\nGet %s project file error!\n\n", si.szSectionName);
			system("pause");
			return FALSE;
		}
		else
		{
			strcpy_s(si.szProject, BUFF_SIZE, project->GetText());
		}

		// #03.06.读取编译器选项/参数  -->> ADD 2019-03-18
		XMLElement* prevoption = source->FirstChildElement("prevoption");
		if (NULL != prevoption)
		{
			strcpy_s(si.szPrevOption, BUFF_SIZE, prevoption->GetText());
		}

		// #03.06.读取编译器选项/参数  -->> ADD 2019-03-18
		XMLElement* postoption = source->FirstChildElement("postoption");
		if (NULL != postoption)
		{
			strcpy_s(si.szPostOption, BUFF_SIZE, postoption->GetText());
		}

		// #03.06.读取源代码编译输出路径
		/*XMLElement* output = source->FirstChildElement("output");
		if (NULL == output)
		{
			printf("\nGet %s output directory error!\n\n", si.szSectionName);
			system("pause");
			return FALSE;
		}
		else
		{
			strcpy_s(si.szOutPath, BUFF_SIZE, output->GetText());
		}*/

		// #03.07.读取源代码编译后的目标文件名称，至少必须有一个
		XMLElement* object = source->FirstChildElement("object");
		while (NULL != object)
		{
			OBJECT obj;

			strcpy_s(obj.szBuffer, BUFF_SIZE, object->Attribute("name"));
			if (object->GetText())
			{
				strcpy_s(obj.szDstPath, BUFF_SIZE, object->GetText());
			}

			si.vecObjName.push_back(obj);

			object = object->NextSiblingElement("object");
		}

		// #03.07.读取源代码编译后的目标文件名称，至少必须有一个
		if (0 == si.vecObjName.size())
		{
			printf("\nGet %s object file name error!\n\n", si.szSectionName);
			system("pause");
			return FALSE;
		}

		g_vecSrcCode.push_back(si);
		source = source->NextSiblingElement("source");
	}

	if (g_vecSrcCode.size() <= 0)
	{
		printf("\nConfiguration file read error!\n\n");
		system("pause");
		return FALSE;
	}

	return TRUE;
}

bool SearchRevision(const char* file_name, unsigned long* revision)
{
	bool bFound = false;
	*revision = (unsigned long)(-1);

	tinyxml2::XMLDocument doc;
	if (XML_SUCCESS == doc.LoadFile(file_name))
	{
		const char* ElementName[] = { "entry", "commit" };

		XMLElement* element = doc.FirstChildElement("info");
		for (int i = 0; element && (i < sizeof(ElementName) / sizeof(ElementName[0])); i++)
		{
			element = element->FirstChildElement(ElementName[i]);
		}

		if (element)
		{
			const char* rev_str = element->Attribute("revision");
			if (rev_str)
			{
				*revision = strtoul(rev_str, NULL, 10);
				bFound = true;
			}
		}
	}

	return bFound;
}

BOOL GetRevision(const char* url, const char* szRevName, DWORD& revision)
{
	BOOL bFound = FALSE;

	// “最后修改的版本”的临时文件路径
	char szDstPath[BUFF_SIZE] = { 0 };
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s\\%s_Rev.xml", g_szCurrPath, DST_PATH, szRevName);

	// 生成源代码的“最后修改的版本”的临时文件
	char szCommand[BUFF_SIZE] = { '\0' };
	sprintf_s(szCommand, BUFF_SIZE, "svn info --xml \"%s\" > \"%s\"", url, szDstPath);
	system(szCommand);

	bFound = SearchRevision(szDstPath, &revision);

	return bFound;
}

BOOL ExportSourceCode(SourceInfo& si)
{
	char szDstPath[BUFF_SIZE] = { 0 };
	char szCommand[BUFF_SIZE] = { '\0' };
	DWORD dwExportRevision = (DWORD)(-1);

	// 把版本号字符串转换成number
	si.dwRevision = strtoul(si.szRevisionString, NULL, 10);
	dwExportRevision = si.dwRevision;

	// 如果没有指定revision，则从SVN上查找“最后修改的版本”
	if ((DWORD)(-1) == si.dwRevision)
	{
		if (0 == si.szRevisionUrl[0])
		{
			if (!GetRevision(si.szSourceUrl, si.szSectionName, si.dwRevision))
			{
				printf("\n%s revision get error!\n\n", si.szSectionName);
				system("pause");
				return FALSE;
			}
			dwExportRevision = si.dwRevision;
		}
		else
		{
			if (!GetRevision(si.szRevisionUrl, si.szSectionName, si.dwRevision))
			{
				printf("\n%s revision get error!\n\n", si.szSectionName);
				system("pause");
				return FALSE;
			}

			if (!GetRevision(si.szSourceUrl, si.szSectionName, dwExportRevision))
			{
				printf("\n%s revision get error!\n\n", si.szSectionName);
				system("pause");
				return FALSE;
			}
		}
	}

	// export源代码
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s\\%s", g_szCurrPath, DST_PATH, si.szSectionName);
	sprintf_s(szCommand, BUFF_SIZE, "svn export -r %d \"%s\" \"%s\" --force", dwExportRevision, si.szSourceUrl, szDstPath);
	system(szCommand);

	return TRUE;
}

BOOL WriteVersionFile(const SourceInfo& si)
{
	FILE *pFile = NULL;

	char szDstPath[BUFF_SIZE] = { 0 };
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s\\%s%s", g_szCurrPath, DST_PATH, si.szSectionName, si.szVerionFile/*, si.szVerionFile*/);

	if (0 != fopen_s(&pFile, szDstPath, "w"))
	{
		printf("\n*********************************************************\n");
		printf("** Write %s Version.c File open failed！ **\n", si.szSectionName);
		printf("*********************************************************\n\n");
		system("pause");
		return FALSE;
	}

	if (pFile)
	{
		vector<VERSION>::const_iterator it = si.vecVersionString.begin();
		for (; it != si.vecVersionString.end(); ++it)
		{
			if (0 != strcmp(it->szIgnore, ""))
			{
				fprintf(pFile, it->szBuffer);
			}
			else
			{
			fprintf(pFile, it->szBuffer, si.dwRevision);
			}
			fprintf(pFile, "\n");
		}

		fclose(pFile);
	}

	return TRUE;
}

BOOL CopySourceCode(const SourceInfo& si)
{
	char szSrcPath[BUFF_SIZE];
	char szDstPath[BUFF_SIZE];
	char szCommand[BUFF_SIZE] = { '\0' };

	sprintf_s(szSrcPath, BUFF_SIZE, "%s%s\\%s", g_szCurrPath, DST_PATH, si.szSectionName);
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s%s%s\\%s", g_szCurrPath, DST_PATH, COMMIT_PATH, OUT_SRC, si.szSectionName);
	sprintf_s(szCommand, BUFF_SIZE, "xcopy \"%s\" \"%s\" /S /I /Y", szSrcPath, szDstPath);
	system(szCommand);

	return TRUE;
}

BOOL BuildSourceCode(const SourceInfo& si)
{
	char szDstPath[BUFF_SIZE] = { 0 };
	char szCommand[BUFF_SIZE] = { '\0' };
	int status;

	// 这里两个问题，为什么不能system()直接执行命令；
	FILE* pFile;
	sprintf_s(szDstPath, BUFF_SIZE, "%s%s\\%s%s", g_szCurrPath, DST_PATH, si.szSectionName, "_building.bat");
	if (0 != fopen_s(&pFile, szDstPath, "w"))
	{
		printf("\n%s Generate the building batch error!\n\n", si.szSectionName);
		system("pause");
		return FALSE;
	}

	sprintf_s(szDstPath, BUFF_SIZE, "%s%s\\%s", g_szCurrPath, DST_PATH, si.szSectionName);
	/*sprintf_s(szCommand, BUFF_SIZE, "\"%s\" -r \"%s%s\" -o \"%s%s\\%s_out.txt\"", si.szKeilPath, szDstPath, si.szProject, szDstPath, si.szOutPath, si.szSectionName);*/
	//system( szCommand );
	//fprintf(pFile, szCommand);
	fprintf(pFile, "\"%s\" ", si.szKeilPath);
	if (0 != strcmp(si.szPrevOption, ""))
	{
		fprintf(pFile, "%s ", si.szPrevOption);
	}
	fprintf(pFile, "\"%s%s\" ", szDstPath, si.szProject);
	if (0 != strcmp(si.szPostOption, ""))
	{
		fprintf(pFile, "%s", si.szPostOption);
	}
	fprintf(pFile, "\r\n");

	fclose(pFile);

	sprintf_s(szCommand, BUFF_SIZE, "\"%s%s\\%s%s\"", g_szCurrPath, DST_PATH, si.szSectionName, "_building.bat");
	status = system(szCommand);

	return TRUE;
}

BOOL CopyDestinationFile(const SourceInfo& si)
{
	bool bFileExist;
	char szFileName[BUFF_SIZE];
	char szDstPath[BUFF_SIZE] = { 0 };

	vector<OBJECT>::const_iterator it = si.vecObjName.begin();
	for (; it != si.vecObjName.end(); ++it)
	{
		sprintf_s(szFileName, BUFF_SIZE, "%s%s\\%s%s", g_szCurrPath, DST_PATH, si.szSectionName, it->szBuffer);
		bFileExist = (0 == _access(szFileName, 0));

		//如果目标文件存在，则复制到新建的版本文件夹中
		if (bFileExist)
		{
			char szCommand[BUFF_SIZE] = { '\0' };

			// 拷贝文件的目标目录
			if (0 == it->szDstPath[0])
			{
				sprintf_s(szDstPath, BUFF_SIZE, "%s%s%s%s\\", g_szCurrPath, DST_PATH, COMMIT_PATH, OUT_BIN);
			}
			else
			{
				sprintf_s(szDstPath, BUFF_SIZE, "%s%s%s%s\\%s\\", g_szCurrPath, DST_PATH, COMMIT_PATH, OUT_BIN, it->szDstPath);
			}

			// 拷贝文件
			sprintf_s(szCommand, BUFF_SIZE, "xcopy \"%s\" \"%s\" /F", szFileName, szDstPath);
			system(szCommand);
		}
		else
		{
			printf("\n*********************************************************\n");
			printf("************* %s 未找到目标文件,无法完成复制！*************\n", si.szSectionName);
			printf("*********************************************************\n\n");
			system("pause");
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CommintVersion(void)
{
	if (0 == _stricmp(g_szDebug, "no commit"))
	{
		return TRUE;
	}

	char szSrcPath[BUFF_SIZE] = { '\0' };
	char szCommand[BUFF_SIZE] = { '\0' };

	char szMsg[BUFF_SIZE] = { '\0' };
	vector<SourceInfo>::iterator it = g_vecSrcCode.begin();
	DWORD dwRevNo = it->dwRevision;
	if (g_vecSrcCode.size() > 1)
	{
		sprintf_s(szMsg, BUFF_SIZE, "(%d", it->dwRevision);
		for (++it; it != g_vecSrcCode.end(); ++it)
		{
			sprintf_s(szMsg, BUFF_SIZE, "%s,%d", szMsg, it->dwRevision);
			if (dwRevNo < it->dwRevision)
			{
				dwRevNo = it->dwRevision;
			}
		}	
		sprintf_s(szMsg, BUFF_SIZE, "%s)", szMsg);
	}

	// 不是打基线的版本提交
	if (0 == strcmp(g_szLabel, ""))
	{
		// 提交文件路径
		sprintf_s(szSrcPath, BUFF_SIZE, "%s%s%s%s", g_szCurrPath, DST_PATH, COMMIT_PATH, OUT_BIN);

		// SVN目录
		char szUrl[BUFF_SIZE];
		if (g_bLeftPadWithZero)
			sprintf_s(szUrl, BUFF_SIZE, "%s/%s%06d%s%s", g_szCommitUrl, g_szPrefix, dwRevNo, g_szSuffix, szMsg);
		else
			sprintf_s(szUrl, BUFF_SIZE, "%s/%s%d%s%s", g_szCommitUrl, g_szPrefix, dwRevNo, g_szSuffix, szMsg);

		// 导入文件到SVN
		if (0 == szMsg[0])
		{
			sprintf_s(szMsg, BUFF_SIZE, "%d", dwRevNo);
		}
		sprintf_s(szCommand, BUFF_SIZE, "svn import \"%s\" \"%s\" -m %s", szSrcPath, szUrl, szMsg);
		system(szCommand);
	}
	// 打基线的版本提交，一般用于向生产发布版本
	else
	{
		// 提交文件路径
		sprintf_s(szSrcPath, BUFF_SIZE, "%s%s%s", g_szCurrPath, DST_PATH, COMMIT_PATH);

		// SVN目录
		char szUrl[BUFF_SIZE];
		sprintf_s(szUrl, BUFF_SIZE, "%s/%s", g_szCommitUrl, g_szLabel);

		// 导入文件到SVN
		sprintf_s(szCommand, BUFF_SIZE, "svn import \"%s\" \"%s\" -m %s", szSrcPath, szUrl, szMsg);
		system(szCommand);
	}

	return TRUE;
}

//int _tmain(int argc, _TCHAR* argv[])
//int main()
int main(int argc, const char* argv[])
{
	int c = -1;
	while (-1 != (c = getopt(argc, (char*const*)argv, "zv")))
	{
		switch (c)
		{
		case 'z':
			g_bLeftPadWithZero = true;
			break;
		case 'v':
			// Only show the version.
			printf("MDKBuild Version: 01.00.00.256\n\n");
			system("pause");
			return 0;
			break;
		default:
			break;
		}
	}

	if (!ReadConfiguration())
	{
		printf("\n*********************************************************\n");
		printf("********Configuration file read error!*******************\n");
		printf("*********************************************************\n\n");
		system("pause");
		return -1;
	}

	if (!CreateWorkingPath())
	{
		printf("\n*********************************************************\n");
		printf("********Configuration file read error!*******************\n");
		printf("*********************************************************\n\n");
		system("pause");
		return -1;
	}

	vector<SourceInfo>::iterator it = g_vecSrcCode.begin();
	for (; it != g_vecSrcCode.end(); ++it)
	{
		if (!ExportSourceCode(*it))
		{
			printf("\n*********************************************************\n");
			printf("\n********%s Export source code error!*********************\n", it->szSectionName);
			printf("\n*********************************************************\n\n");
			system("pause");
			return -1;
		}

		if (!WriteVersionFile(*it))
		{
			printf("\n*********************************************************\n");
			printf("*************%s General version.c file failed！************\n", it->szSectionName);
			printf("*********************************************************\n\n");
			system("pause");
			return -1;
		}

		if (!CopySourceCode(*it))
		{
			printf("\n*********************************************************\n");
			printf("*************%s Copy source code failed！******************\n", it->szSectionName);
			printf("*********************************************************\n\n");
			system("pause");
			return -1;
		}

		if (!BuildSourceCode(*it))
		{
			printf("\n*********************************************************\n");
			printf("*************%s Build Source Code failed！*****************\n", it->szSectionName);
			printf("*********************************************************\n\n");
			system("pause");
			return -1;
		}

		if (!CopyDestinationFile(*it))
		{
			printf("\n*********************************************************\n");
			printf("*************%s Copy destination file failed！*************\n", it->szSectionName);
			printf("*********************************************************\n\n");
			system("pause");
			return -1;
		}
	}

	if (!CommintVersion())
	{
		printf("\n*********************************************************\n");
		printf("************* Scritpt execute failed！******************\n");
		printf("*********************************************************\n\n");
		system("pause");
		return -1;
	}

	printf("\n*********************************************************\n");
	printf("************* Scritpt execute success！******************\n");
	printf("*********************************************************\n\n");
	system("pause");

	return 0;
}
