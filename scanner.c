/*
sanner.c
this programming is the scanner's bady
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<dirent.h>
#include<sys/time.h>
#include<fcntl.h>
#include<sys/select.h>
#include<pthread.h>
#include<netdb.h>
#include"scanner.h"

int main(int argc, char * argv[])
{
  //判断输入参数是否正确
  if(argc != 2)
    DieWithUserMessage("Parameter(s)", "<sudo ./scanner x.x.x.x> <sudo ./scanner x.x.x.x-x>");

  //建立一个当前目录下的扫描日志scanlog
  DIR * dir = NULL;
	if((dir=opendir("./scanlog")) == NULL)
	{
		//若scanlog不存在,创建目录
		system("mkdir ./scanlog/");
	}
	else
	{
		//scanlog存在,删除目录下的所有文件
		closedir(dir);
		system("rm  ./scanlog/*");
	}
//主机默认系统端口位于1～1023
    unsigned startPort = DEFAULT_START_PORT;
    unsigned overPort = DEFAULT_END_PORT;

  // 由输入参数构造扫描IPaddress
  char ipAddrPre[81] = {0};     //要扫瞄的地址前缀
  int j;
  int len =strlen(*(argv + 1));

//这个循环负责处理输入，获得要扫瞄的局域网前缀，前24位
  for(int j = 0 , flag = 0; j < len ; j++)
  {

          ipAddrPre[j] = *(*(argv + 1) + j);
        //flag++;

    if(ipAddrPre[j] == '.')
    {
      flag++;
      if(flag == 3)
      {
        ipAddrPre[j] = '.';       //上面的处理会不停在最后一 . 所以要加上
        ipAddrPre[j + 1] = '\0';  //是按字符串处理的，所以要加上\0
        break;
      }
    }

  }
  //设定C类局域网192.168.X.0/24的所有IP，0～256
  char ip[256][81] = {0};
  char ipPostfix[81] = {0};       //后缀字符数组

  int i;

  //线程数组，每个线程扫描一个IP

  pthread_t tid[256];

  //循环控制线程
  for(i = 0 ; i <=255 ; i++)
{
  memset(ip[i], 0, sizeof(ip[i]));
  memset(ipPostfix, 0, sizeof(ipPostfix));

  //构造正确扫描地址 输入前缀 + 最后一段 eg. 192.168.60.xxx
  strcat(ip[i], ipAddrPre);     //最先加入前缀
    sprintf(ipPostfix, "%d", i);
      strcat(ip[i], ipPostfix);    //将格式化后的后缀添加到前缀后面

  //给每一个ip分配一个线程
  pthread_create(&tid[i], NULL, (void *)CheckDestOpenPort, (void *) &ip[i]);
  //有的编译器可能会报错，类型转换失败，但是直接make却正常
  //等待该线程结束，设置为 非阻塞，可立即返回执行
  pthread_detach(tid[i]);
}
  //主线程休眠5sec,等待子线程结束
  sleep(5);
  //输出scanlog扫描日志中的记录信息
  system("cat ./scanlog/*");
  //使用system命令调用cat指令
  return 0;
}

//deal with error from user
void DieWithUserMessage(const char * msg, const char * detail)
{
  fputs(msg, stderr);
   fputs(": ", stderr);
    fputs(detail, stderr);
      fputc('\n', stderr);
        exit(1);
}
//deal with error form system
void DieWithSystemMessage(const char * msg)
{
  perror(msg);
  exit(1);
}
//check the opened Port
void * CheckDestOpenPort(void * argv)
{
  in_port_t destPort;
  unsigned int startPort = DEFAULT_START_PORT;
	unsigned int overPort = DEFAULT_END_PORT;

  int port_flag = 0;  //端口开关标志，0为关闭，非0为打开

  char *destIP = (char *)argv;

  //construct the dest host adress structure
  struct sockaddr_in destAddr;        //dest address
  memset(&destAddr, 0, sizeof(destAddr));     //zero out structure
  destAddr.sin_family = AF_INET;    //ipv4 address family
  //convert address
  int rtnVal = inet_pton(AF_INET, destIP, &destAddr.sin_addr.s_addr);
/*  if(rtnVal == 0)
    DieWithUserMessage("inet_pton()", "invalid address string");
    else if(rtnVal < 0)
      DieWithSystemMessage("inet_pton() failed");
*/
  int connectStatus = -1;
  struct servent * dest = NULL;
  /*
  struct servent
   {
    char  *s_name;
    char **s_aliases;
    int    s_port;
    char  *s_proto;
   }
  */

  //摄制等待扫描一个IP的时间戳

  struct timeval startTime, overTime;

  float timeCost; //统计一个在线主机扫描的时间

  //获取线程开始时间起点
  gettimeofday(&startTime, 0);

  int fd = -1;

  char logpath[255] = {0};  //日志路径
  char buf[255] = {0};    //写入缓冲
  char portStr[81] = {0};  //端口

  int setflag = 0;

  //开始从0～1024端口扫描
  for(destPort = startPort ; destPort <= overPort ; destPort++)
  {
    //create a reliable ,stream socket using TCP
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0)
    {
      DieWithSystemMessage("socket() failed");
    }
  destAddr.sin_port = htons(destPort);    //dest in_port_t


  //establish the conection to the dest server
  connectStatus = connect(sock, (struct sockaddr *) &destAddr, sizeof(destAddr));

  if(connectStatus != 0 )
    {
      //connect()失败；
      close(sock);
    }
    else
    {
      //建立连接，此端口开放，主机在线
      port_flag++;  //更改标志
      if(port_flag != 0)
      {
        //记录该主机信息到scanlog下
        memset(buf, 0, sizeof(buf));
         memset(logpath, 0, sizeof(logpath));

            strcat(logpath, "./scanlog/");  //写入日志路径
              strcat(logpath,destIP);     //写入当前Ip

        fd = open(logpath, O_RDWR | O_CREAT | O_TRUNC, 0777);
        if(fd == -1)
				{
					perror("open");
					return NULL;
				}
        //写入主机Ip
				strcat(buf,"IP:");
		  		strcat(buf,destIP);
			    	strcat(buf,"\tHost is up!\n");
			      	write(fd, buf, strlen(buf));

        //写入主机名
				memset(buf,0,sizeof(buf));
				strcat(buf,"HostName:");

        //创建honst entry结构体指针
        struct hostent * hostEntPtr;
        /*
        #include <netdb.h>
        #include <sys/socket.h>	/* for AF_INET
        struct hostent *gethostbyaddr(const void *addr,socklen_t len, int type);
        const void *addr：参数addr不是void*类型， 而是一个真正指向含有IPv4或IPv6地址的结构in_addr或in6_addr；
        socklen_t len：第一个参数的结构大小，对于 IPv4地址为4,对于IPv6地址为16；
        int family：AF_INET或AF_INET6；
        */
        hostEntPtr = gethostbyaddr((void *)&destAddr.sin_addr, 4 ,AF_INET);
        //调用该函数访问该结构体
        if(hostEntPtr == NULL)
        {
          printf("\ngethostnamebyaddr error for addr:%s\n",destIP);
					 printf("please add this ip and hostname into file:\"/etc/hosts\"\n");
					  strcat(buf,"Unknown");
        }
        else
        {
        /*  struct hostent
            {
	             char * h_name; 		/ *主机的正式名称* /
	             char ** h_aliases; 	/ *主机的别名,一个主机可以有多个别名* /
	             int h_addrtype; 	/ *主机地址类型:IPV4-AF_INET* /
	             int h_length; 		/ *主机地址字节长度，对于IPv4是四字节，即32位* /
	             char ** h_addr_list; / *地址列表* /
            }
          */
          strcat(buf, hostEntPtr->h_name);
        }
        strcat(buf, "\n");
         write(fd, buf, strlen(buf));    //将缓冲中的数据写入sacnlog

        //写入该次扫描日期
        memset(buf, 0, sizeof(buf));
         getTime(buf);
          write(fd, buf, sizeof(buf));

        //写入信息i栏 eg. port  status  serveice
        memset(buf, 0, sizeof(buf));
         strcat(buf, "port\tstatus\tservice\n");
          write(fd, buf, sizeof(buf));
      }
      //写入端口
      memset(buf, 0, sizeof(buf));
        sprintf(portStr, "%d", destPort);
          strcat(buf, portStr);
            strcat(buf, "/ttcp\topen\t");
      //eg: xxx tcp open
      dest = getservbyport(htons(destPort), "tcp"); //访问/etc/hosts下的结构体，获取信息
      if(!dest)
      {
        strcat(buf,"Unknown");
      }
      else
      {
        strcat(buf,dest->s_name);
				strcat(buf,"  ");
				int i = -1;
				for(i=0;dest->s_aliases[i];i++)
				{
					strcat(buf,dest->s_aliases[i]);
					strcat(buf,"  ");
				}
      }
      strcat(buf,"\n");
      write(fd, buf, strlen(buf));

      close(sock);  //每建立一个套接字，最后都要关闭，否则系统一直占用
    }

    //统计整个过程的时间花费
    gettimeofday(&overTime, 0);
    timeCost = TIME *(overTime.tv_sec - startTime.tv_sec) + overTime.tv_usec - startTime.tv_usec;
    //tv_sec 微秒，tv_usec毫秒
    timeCost /= 1000;
    if(timeCost < 5000 && destPort == overPort)
    {
      memset(buf, 0, sizeof(buf));
       memset(logpath, 0, sizeof(logpath));

          strcat(logpath, "./scanlog/");  //写入日志路径
            strcat(logpath,destIP);     //写入当前Ip

      fd = open(logpath, O_RDWR | O_CREAT | O_TRUNC, 0777);
      if(fd == -1)
      {
        perror("open");
        return NULL;
      }
      //写入主机Ip
      strcat(buf,"IP:");
        strcat(buf,destIP);
          strcat(buf,"\tHost is up!\n");
            write(fd, buf, strlen(buf));

      //写入主机名
      memset(buf,0,sizeof(buf));
      strcat(buf,"HostName:");

      //创建honst entry结构体指针
      struct hostent * hostEntPtr;
      /*
      #include <netdb.h>
      #include <sys/socket.h>	/* for AF_INET
      struct hostent *gethostbyaddr(const void *addr,socklen_t len, int type);
      const void *addr：参数addr不是void*类型， 而是一个真正指向含有IPv4或IPv6地址的结构in_addr或in6_addr；
      socklen_t len：第一个参数的结构大小，对于 IPv4地址为4,对于IPv6地址为16；
      int family：AF_INET或AF_INET6；
      */
      hostEntPtr = gethostbyaddr((void *)&destAddr.sin_addr, 4 ,AF_INET);
      //调用该函数访问该结构体
      if(hostEntPtr == NULL)
      {
        printf("\ngethostnamebyaddr error for addr:%s\n",destIP);
         printf("please add this ip and hostname into file:\"/etc/hosts\"\n");
          strcat(buf,"Unknown");
      }
      else
      {
      /*  struct hostent
          {
             char * h_name; 		/ *主机的正式名称* /
             char ** h_aliases; 	/ *主机的别名,一个主机可以有多个别名* /
             int h_addrtype; 	/ *主机地址类型:IPV4-AF_INET* /
             int h_length; 		/ *主机地址字节长度，对于IPv4是四字节，即32位* /
             char ** h_addr_list; / *地址列表* /
          }
        */
        strcat(buf, hostEntPtr->h_name);
      }
      strcat(buf, "\n");
       write(fd, buf, strlen(buf));    //将缓冲中的数据写入sacnlog

      //写入该次扫描日期
      memset(buf, 0, sizeof(buf));
       getTime(buf);
        write(fd, buf, sizeof(buf));

      //写入信息i栏 eg. port  status  serveice
      memset(buf, 0, sizeof(buf));
       strcat(buf, "port\tstatus\tservice\n");
        write(fd, buf, sizeof(buf));
    //}
    //写入端口
    memset(buf, 0, sizeof(buf));
    //  sprintf(portStr, "%d", destPort);
        //strcat(buf, portStr);
          strcat(buf, "/tall port is closed !\t");
    //eg: xxx tcp open
    dest = getservbyport(htons(destPort), "tcp"); //访问/etc/hosts下的结构体，获取信息
    if(!dest)
    {
      strcat(buf,"Unknown");
    }
    else
    {
      strcat(buf,dest->s_name);
      strcat(buf,"  ");
      int i = -1;
      for(i=0;dest->s_aliases[i];i++)
      {
        strcat(buf,dest->s_aliases[i]);
        strcat(buf,"  ");
      }
    }
    strcat(buf,"\n");
    write(fd, buf, strlen(buf));

    close(sock);
      return;
    }
    if(timeCost > 5000)
    {
      //该扫描时间如果大于5S，认为该主机down
          break;   //结束循环，退出扫描
    }
}
// 记录对该Ip的扫描时间
    gettimeofday(&overTime, 0);
    timeCost = TIME *(overTime.tv_sec - startTime.tv_sec) + overTime.tv_usec - startTime.tv_usec;
    //tv_sec 微秒，tv_usec毫秒
    timeCost /= 1000;

    char time[81] = {0};
	   sprintf(time,"%f",timeCost);

	    memset(buf,0,sizeof(buf));
	     strcat(buf,"Cost time: ");
	      strcat(buf, time);
	       strcat(buf,"ms\n\n");

	        write(fd,buf,strlen(buf));
	         close(fd);
	          return NULL;
}
//获取时间并规范化表示
void getTime(char *buf)
{
	time_t timep;
	struct tm *p;
	char temp[81] = {0};

	time(&timep);
	p = localtime(&timep);

	memset(buf,0,sizeof(buf));
	 memset(temp,0,sizeof(temp));
	  sprintf(temp,"%d",(1900 + p->tm_year));
	   strcat(buf,temp);
	    strcat(buf,"-");

	memset(temp,0,sizeof(temp));
	 sprintf(temp,"%d",(1 + p->tm_mon));
	  strcat(buf,temp);
	   strcat(buf,"-");

	memset(temp,0,sizeof(temp));
	 sprintf(temp,"%d",(p->tm_mday));
	  strcat(buf,temp);
	   strcat(buf," ");

	memset(temp,0,sizeof(temp));
	 sprintf(temp,"%d",p->tm_hour);
	  strcat(buf,temp);
	   strcat(buf,":");

	memset(temp,0,sizeof(temp));
	 sprintf(temp,"%d", p->tm_min );
	  strcat(buf,temp);
	   strcat(buf,":");

	memset(temp,0,sizeof(temp));
	 sprintf(temp,"%d", p->tm_sec );
	  strcat(buf,temp);
	   strcat(buf,"\n");
}
