#define  MAXPORT 26
//process the hostaddr to machine can hand
char * HandIpadrr(const char * hostadrr, int * Addrcnt);
//test for corrent argument
void DieWithUserMessage(const char * msg, const char * detail);
//for system
void DieWithSystemMessage(const char * msg);
//test the host pen which port
int  CheckDestOpenPort(int port, char * destIP);
int scan(char * , int );
void *scanThreadFunc(void *args);
#define DEFAULT_THREAD_NUM 8
#define DEFAULT_START_PORT 1
#define DEFAULT_END_PORT 1024
int PortCnt = 0;      //count the port that is openning
int Port[MAXPORT];
typedef struct
{
    int pthreadNum;
    char * ip;
    int port;
    int end;
} ScanThreadArgs, * ScanThreadArgsPtr;
