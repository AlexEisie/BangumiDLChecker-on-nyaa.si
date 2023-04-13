#include <stdio.h>
#include <regex.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <unistd.h>
#include <vector>
using namespace std;

bool isNew(string torrent, ifstream& file)
{
	file.clear();	//！！！到了文件尾以后需要先清除指针状态才能定位！！！
	file.seekg(ios_base::beg);
	char linet[1024];
	while (file.getline(linet, 1023))
	{
		string line(linet);
		if (torrent == line) return 0;
	}
	return 1;
}

int main()
{
	/*
	//获取当前路径
	char *cwd;
	if((cwd = getcwd(NULL, 0)) == NULL)	perror("getcwd error");
	//生成文件保存绝对路径
	string downloaddir(cwd);
	downloaddir+="/downloads/";
	*/

	//定义编译正则表达式，a用于提取链接，b用于处理config.cfg
    regex_t a;
    int ret = regcomp(&a, "/download/[^\"]+.torrent", REG_EXTENDED);
    if(ret){
        cout<<"Regex a compilation failed"<<endl;
        return 1;
    }
    regmatch_t maga[1];

	regex_t b;
	ret = regcomp(&b, "[^|]+", REG_EXTENDED);
	if (ret) {
		cout << "Regex a compilation failed" << endl;
		return 1;
	}
	regmatch_t magb[1];
	
	//重复执行死循环
	while(1)
	{
	//调用爬虫
	system("python3 webclawer.py");

	//打开config.cfg文件
	ifstream cfg;
	cfg.open("config.cfg");
	char cfglinet[1024];
	if(!cfg.getline(cfglinet,1023)) 
	{
		cout<<"No config in config.cfg!!!"<<endl<<"You can add a new rule like: name|URL|uploaddir"<<endl;
		return 0;
	}

	//读取downloadedlist.txt
	ifstream DLLin("downloadedlist.txt");
	ofstream DLLout("downloadedlist.txt", ios::app);
	if (!DLLin || !DLLout)
	{
		cout << "Filed to open downloadedlist.txt, if this file doesnt exist please create one manually(which is allowed to be empty" << endl;
		return -1;
	}

	//待下载队列
	vector<string> queue;

	//逐行扫描cfg并保存到cfgline,进行该行配置的下载任务
	cfglinet[1023]=0;
	string cfgline(cfglinet);
	
	while (1)
	{
		string name;
		string uploaddir;
		//分割config.cfg每一行的三部分并且储存和显示
		if (!regexec(&b, cfgline.c_str(), 1, magb, 0))
		{
			name=string(cfgline, magb[0].rm_so, magb[0].rm_eo);
			cout << "name="<<string(cfgline, magb[0].rm_so, magb[0].rm_eo) << endl;
		}
		else
		{
			cout << "Failed to get name part in config.cfg" << endl;
			return -1;
		}
		uploaddir = string(cfgline, magb[0].rm_eo + 1, cfgline.length());
		if (!regexec(&b, uploaddir.c_str(), 1, magb, 0))
		{
			//uploaddir = string(uploaddir, magb[0].rm_eo+1, uploaddir.length()-1);
			cout << "URL=" << string(uploaddir, magb[0].rm_so, magb[0].rm_eo) << endl;
			uploaddir="\""+string(uploaddir, magb[0].rm_eo+1, uploaddir.length()-1)+"\"";
			cout << "uploaddir=" <<uploaddir<< endl;
		}
		else
		{
			cout << "Failed to get URL and uploaddir part in config.cfg" << endl;
			return -1;
		}

		
		
		//读取单个html文件并且过滤出种子链接
		string htmld("html/");
		htmld += name;
		htmld += ".html";
		ifstream is;
		is.open(htmld.c_str());
		char linet[1024];
		is.getline(linet, 1023);
		linet[1023] = 0;
		string line(linet);
		string torrent;
		while (1)
		{
			//匹配种子链接并且显示
			if (!regexec(&a, line.c_str(), 1, maga, 0))
			{
				torrent = "https://nyaa.si" + string(line, maga[0].rm_so, maga[0].rm_eo - 14);
				//判断是否为新种子，如果是则写入downloadedlist.txt并且保存到待下载队列，如果不是则跳过
				if (isNew(torrent, DLLin))
				{
					cout << "*" << torrent << "  is a new one, added to the queue"<<endl;
					DLLout << torrent << endl;
					queue.push_back(torrent);
				}
				else
					cout << torrent << "  is an old one, skipped"<<endl;
			}
			//读取html文件的下一行
			line.clear();
			if (!is.getline(linet, 1023)) break;
			line = string(linet);
		}
		is.close();

		//获取步骤到此结束
		//调用下载模块
		cout << "该行配置读取已完成，开始下载" << endl;
		int dlnum = 0;
		int vecvol = queue.size();
		string wget_command;
		string aria2c_command;
		while (dlnum < vecvol)
		{
			cout<<"开始下载:"<<queue[dlnum]<<endl;
			wget_command = "wget " + queue[dlnum] + " -P ./torrent -nv";
			system(wget_command.c_str());
			queue[dlnum].erase(0,queue[dlnum].find_last_of("/")+1);		//去除前缀
			aria2c_command = "aria2c ./torrent/" + queue[dlnum] + " --dir ./downloads --seed-time 0 --bt-stop-timeout 300";
			system(aria2c_command.c_str());
			dlnum++;
		}

		if(!queue.empty())
		{	
			//调用上传模块
			cout<<"开始将文件上传到:"<<uploaddir<<endl;
			string upload_command;
			upload_command="rclone move ./downloads "+uploaddir+" -P";
			cout<<upload_command<<endl;
			system(upload_command.c_str());
			cout<<"上传完成"<<endl;
		}
		else
		{
			cout<<"未找到新内容"<<endl;
		}
		queue.clear();
		//读取config.cfg的下一行
		cfgline.clear();
		if (!cfg.getline(cfglinet, 1023)) break;
		cfgline = (string)cfglinet;
	}
	queue.clear();
	DLLin.close();
	DLLout.close();
	cfg.close();
	
	//6分钟后执行下次死循环
	cout<<endl<<"******已经完成任务,6分钟后再次开始*******"<<endl;;
	sleep(6*60);
	}
    regfree(&a);
	regfree(&b);
    return 0;
}


