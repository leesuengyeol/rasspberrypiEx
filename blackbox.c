#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h> 	 //directory ctrl
#include <sys/stat.h> 	 //file stat
#include <sys/vfs.h>		 //file system
#include <sys/types.h>   //variable types
#include <sys/wait.h>	 //process wait
#include <errno.h>
#include <signal.h>
//time
#include <time.h>
#include <sys/time.h>


#define DEBUG 
#define VIDEOTIME 59500  //unit: msec  (촬영시간)
#define p_width 640
#define p_height 480

const char *MMOUNT = "/proc/mounts";  //위치
pid_t pid; //멀티프로세서

struct f_size
{
    long blocks; //전체 블럭수
    long avail;  //사용가능한 블럭수
};

typedef struct
{
    FILE *fp;               // 파일 스트림 포인터    
    char devname[80];       // 장치 이름
    char mountdir[80];      // 마운트 디렉토리 이름
    char fstype[12];        // 파일 시스템 타입
    long f_type;
    long f_blocks;	    // Total Block count
    long f_bavail;	    // unused block count(Normal User)
    
	struct f_size size;     // 파일 시스템의 총크기/사용율 
} MOUNTP;

MOUNTP *dfopen()
{
    MOUNTP *MP;

    // /proc/mounts 파일을 연다.
    MP = (MOUNTP *)malloc(sizeof(MOUNTP));
    if(!(MP->fp = fopen(MMOUNT, "r")))
    {
        return NULL;
    }
    else
        return MP;
}

MOUNTP *dfget(MOUNTP *MP)
{
    char buf[256];
    char *bname;
    char null[16];
    struct statfs lstatfs;
    struct stat lstat; 
    int is_root = 0;

    // /proc/mounts로 부터 마운트된 파티션의 정보를 얻어온다.
    while(fgets(buf, 256, MP->fp)) //fgets->  한라인씩 파일의 정보를 읽어온다 buf에 256 만큼
    {
       // is_root = 0;
        sscanf(buf, "%s%s%s",MP->devname, MP->mountdir, MP->fstype);  //sscanf -> 버퍼에 저장
        if (strcmp(MP->mountdir,"/") == 0) is_root=1;
        if (stat(MP->devname, &lstat) == 0 || is_root)//실제 device가 읽어진다면 0
        {
            if (strstr(buf, MP->mountdir) && S_ISBLK(lstat.st_mode) || is_root) //
            {
                // 파일시스템의 총 할당된 크기와 사용량을 구한다.        
                statfs(MP->mountdir, &lstatfs); //파일시스템의 정보를 가져옴
                MP->size.blocks = lstatfs.f_blocks * (lstatfs.f_bsize/1024); 
                MP->size.avail  = lstatfs.f_bavail * (lstatfs.f_bsize/1024);
				/*MP->f_type	= lstatfs.f_type;
				MP->f_bsize     = lstatfs.f_bsize;
				MP->f_blocks	= lstatfs.f_blocks;
				MP->f_bavail	= lstatfs.f_bavail;
				MP->f_files     = lstatfs.f_files;
				MP->f_bfree	= lstatfs.f_bfree;
				MP->f_ffree	= lstatfs.f_ffree;*/
                return MP;
            }
        }
    }
    rewind(MP->fp);
    return NULL;
}

int dfclose(MOUNTP *MP)
{
    fclose(MP->fp);
}

static void sig_handler(int signum)
{
	kill(pid,SIGKILL);
	exit(1);
}


int main(int argc,char **argv)
{
	time_t UTCtime;
	struct tm *tm; // 시간 저장할 구조체
	char dirNamebuf[BUFSIZ]; //디렉토리 이름
	char fileNamebuf[BUFSIZ]; // 파일 이름
	char tempbuf[25]; //임시 저장공간
	char cmdbuf[BUFSIZ]; 
	//FHD 1920 X 1080  , HD 1280 X 720
	int width; 	//영상 size
	int height; //영상 size
	int status;
	float remain;
	char fullPath[50];
	MOUNTP *MP;
	
	signal(SIGINT,(void*)sig_handler);
	

	//argv[1] == 0, FHD
	if(!strcmp("0",argv[1]))
	{
		width = 1920;
		height = 1080;

	}
	//argv[1] == 1 ,  HD
	else if(!strcmp("1",argv[1]))
	{
		width =1280;
		height =720;
	}
	//failed
	else
	{
		printf("Error: argv[1] is not vaild\n");
		return -1;
	}
	
#ifdef DEBUG
	printf("width: %d, height:%d \n",width,height);
#endif
	
	while(1)
	{
		//커널로부터 시간을 읽어온다
		time(&UTCtime);

		//tm구조체 : 분해된 시간으로 저장
		tm = localtime(&UTCtime);

		//디렉토리명을 생성
		strftime(dirNamebuf,sizeof(dirNamebuf),"%Y%m%d%H",tm); //format = 년원일시 , tm 구조체로부터  dirNamebuf 에 저장
		
		//파일명 생성
		strftime(tempbuf, sizeof(tempbuf), "%Y%m%d_%H%M%S.h264",tm);
		sprintf(fileNamebuf,"%s/%s",dirNamebuf,tempbuf);  // 20190316/20190316_1314.h264 디렉토리명/파일명 <- 저장

#ifdef DEBUG
	printf("dirNamebuf : %s \n",dirNamebuf);
	printf("filenamebuf : %s \n",fileNamebuf);
#endif
		
		struct dirent *de = NULL; // 주소값은 없지만 NULL로 초기화하여 오류방지
		if((MP=dfopen())==NULL)
		{
			printf("Error: dfopen()\n");
			return -1;
		}
		dfget(MP);
		//남아있는 디스크 공간이 5%미만이라면
		//가장 오래된 디렉토리를 삭제한다
		remain = (((float)MP->size.avail/(float)MP->size.blocks) *100);
#ifdef DEBUG
				printf("blocks : %ld\n",MP->size.blocks); // 블록전체크기
				printf("avail : %ld \n",MP->size.avail);  //사용가능한 블록크기
				printf("remain(%):%f\n",remain);		  //사용가능한 블록 퍼센테이지
#endif
		if(remain<5)
		{
			//TODO : 가장 오래된 디렉토리를 찾아서 삭제 추가

		}
		dfclose(MP);
		
		//현재 시간으로 디렉토리 생성
		//mkdir error 처리 (errno.h)
		//EACCES :생성하려는 부모 디렉토리에 쓰기권한이 없음
		//EFAULT : pathname이 접속가능한 메모리가 아님
		//ENOTDIR : pathname에 디렉토리가 아닌 것이 있음
		if(mkdir(dirNamebuf , 0666)==-1)
		{
			if(errno !=EEXIST)  //??
					return -1;
		}

		chmod(dirNamebuf , 0777);
		
		sprintf(cmdbuf,"raspivid -p 0,0,%d,%d -w %d -h %d -t %d -o %s",p_width,p_height,width,height,VIDEOTIME,fileNamebuf); //-hf -vf
#ifdef DEBUG
		printf("cmdbuf : %s\n" ,cmdbuf);
#endif
		
		//자식프로세스 생성
		pid =fork();

		//부모 프로세스
		if(pid>0)
		{
			//none
		}
		//자식 프로세서
		else if(pid ==0)
		{
				//sprintf(fullPath, "/home/pi/blackbox/%s",cmdbuf);
				//execlp("blackbox","raspivid -p 0,0,320,240, -w 640 -h 480 -t 10000 -o a.h264",NULL);
				system(cmdbuf);
				exit(1);
		}

		wait(NULL);
	}
	

	return 0;
}
