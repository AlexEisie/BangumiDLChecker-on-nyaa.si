import urllib.request
import os
import ssl

ssl._create_default_https_context = ssl._create_stdlib_context

# 定义保存函数
def saveFile(data,path):
    f = open(path, 'wb')
    f.write(data)
    f.close()
#设置请求头
headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) '
                         'Chrome/51.0.2704.63 Safari/537.36'}

if not os.path.exists("html"):
    os.makedirs("html")
if not os.path.exists("downloads"):
    os.makedirs("downloads")
if not os.path.exists("torrent"):
    os.makedirs("torrent")

#打开config.cfg文件并且逐行读取链接和名称，下载html保存到相应的html文件中
with open('config.cfg','r') as configf:
	for line in configf:
		name, url, Dir = line.strip().split('|')
		if len(name)==0 or len(url)==0 or len(Dir)==0:
			print("Something wrong with config.cfg, please check it")
			exit()
		req = urllib.request.Request(url=url, headers=headers)
		res = urllib.request.urlopen(req)
		data = res.read()
		# 保存到文件
		path='html/'+name+'.html'
		saveFile(data,path)
		data = data.decode('utf-8')
		# 打印检查
		print(data)
