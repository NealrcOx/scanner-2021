
#define TIME 1000000    //微秒和秒的数量级
#define DEFAULT_START_PORT 1  //定义开始端口
#define DEFAULT_END_PORT 1024 //定义结束端口

//test for corrent argument
void DieWithUserMessage(const char * msg, const char * detail);
//for system
void DieWithSystemMessage(const char * msg);
//test the host pen which port
void * CheckDestOpenPort(void * destIP);
//deal the time right
void getTime(char *buf);
