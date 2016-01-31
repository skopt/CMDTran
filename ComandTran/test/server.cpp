#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>

int Login(int sock,int type);
void FrameInit(char* frame, int len, unsigned char code);
void SendDataSepecificLen(int sock, int len);
void RecvContent(int sock);

int main()
{
	int sock = 0;
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("socket creat faild\n");
		return 0;
	}
	//fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0)|O_NONBLOCK);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(6000);

	if(bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0)
	{
		printf("failed to connect\n");
		return 0;
	}
    if(listen(sock, 1024) == -1)
	{
		printf("listen faild\n");
		return 0;
	}

    RecvContent(sock);
   
    return 1;
}
void RecvContent(int sock)
{
	sockaddr_in v_ClntAddr;	
	socklen_t addrlen = sizeof(sockaddr_in);
	int confd = accept(sock, (struct sockaddr *)&v_ClntAddr,&addrlen);
    char* buff[2048];
	int ret = 0;
	while(true)
	{
        ret = read(confd, buff, 2048);
		if(ret <= 0)
			return;
	}
}
int Login(int sock,int type)
{
	char login_buff[8];
	FrameInit(login_buff, 8, 0x00);
	login_buff[4] = 0x00;
	login_buff[5] = 0x00;
	login_buff[6] = 0x01;
	if(send(sock, login_buff, 8, 0) != 8)
    {
		printf("login failed\n");
        return -1;
    }
    return 0;
}

void FrameInit(char* frame, int len, unsigned char code)
{
    frame[0] = 0x7E;
    frame[1] = (unsigned char) (len / 256);
    frame[2] = (unsigned char) (len % 256);
    frame[3] = code;
    frame[len - 1] = 0xA5;
}

void SendDataSepecificLen(int sock, int len)
{
	if(len > 1000)
		return;
    char send_buff[1024];
    for(int i = 4; i < len - 1; ++i)
	{
        send_buff[i] = (char) i%256;
	}
	FrameInit(send_buff, len + 5, 0x10);
	int ret = 0,send_count = 0,  total = len + 5, send_total = 0;
	timeval last_time, current_time;
	gettimeofday(&last_time, NULL);
	while(true)
	{
        ret = send(sock, send_buff + send_count, total - send_count, 0);
		if(ret < 0)
		{
        	timespec spec;
			spec.tv_sec = 0;
			spec.tv_nsec = 1000*10;
			nanosleep(&spec, NULL);
			continue;
		}
		else if(ret > 0 && ret == total - send_count)
			send_count = 0;
		else
		{
			send_count += ret;
			timespec spec;
			spec.tv_sec = 0;
			spec.tv_nsec = 1000*10;
			nanosleep(&spec, NULL);
			//printf("send count is %d\n", ret);
			continue;
		}

		send_total++;
		gettimeofday(&current_time, NULL);
		if(current_time.tv_sec - last_time.tv_sec > 1)
		{
			float qps = send_total / (current_time.tv_sec - last_time.tv_sec);
			printf("qps=%f\n", qps);
			send_total = 0;
			last_time = current_time;
		}
	}

}
