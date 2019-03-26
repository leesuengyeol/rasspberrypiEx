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
#define VIDEOTIME 59500  //unit: msec  (�Կ��ð�)
#define p_width 640
#define p_height 480

const char *MMOUNT = "/proc/mounts";  //��ġ
pid_t pid; //��Ƽ���μ���

struct f_size
{
    long blocks; //��ü ����
    long avail;  //��밡���� ����
};

typedef struct
{
    FILE *fp;               // ���� ��Ʈ�� ������    
    char devname[80];       // ��ġ �̸�
    char mountdir[80];      // ����Ʈ ���丮 �̸�
    char fstype[12];        // ���� �ý��� Ÿ��
    long f_type;
    long f_blocks;	    // Total Block count
    long f_bavail;	    // unused block count(Normal User)
    
	struct f_size size;     // ���� �ý����� ��ũ��/����� 
} MOUNTP;

MOUNTP *dfopen()
{
    MOUNTP *MP;

    // /proc/mounts ������ ����.
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

    // /proc/mounts�� ���� ����Ʈ�� ��Ƽ���� ������ ���´�.
    while(fgets(buf, 256, MP->fp)) //fgets->  �Ѷ��ξ� ������ ������ �о�´� buf�� 256 ��ŭ
    {
       // is_root = 0;
        sscanf(buf, "%s%s%s",MP->devname, MP->mountdir, MP->fstype);  //sscanf -> ���ۿ� ����
        if (strcmp(MP->mountdir,"/") == 0) is_root=1;
        if (stat(MP->devname, &lstat) == 0 || is_root)//���� device�� �о����ٸ� 0
        {
            if (strstr(buf, MP->mountdir) && S_ISBLK(lstat.st_mode) || is_root) //
            {
                // ���Ͻý����� �� �Ҵ�� ũ��� ��뷮�� ���Ѵ�.        
                statfs(MP->mountdir, &lstatfs); //���Ͻý����� ������ ������
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
	struct tm *tm; // �ð� ������ ����ü
	char dirNamebuf[BUFSIZ]; //���丮 �̸�
	char fileNamebuf[BUFSIZ]; // ���� �̸�
	char tempbuf[25]; //�ӽ� �������
	char cmdbuf[BUFSIZ]; 
	//FHD 1920 X 1080  , HD 1280 X 720
	int width; 	//���� size
	int height; //���� size
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
		//Ŀ�ηκ��� �ð��� �о�´�
		time(&UTCtime);

		//tm����ü : ���ص� �ð����� ����
		tm = localtime(&UTCtime);

		//���丮���� ����
		strftime(dirNamebuf,sizeof(dirNamebuf),"%Y%m%d%H",tm); //format = ����Ͻ� , tm ����ü�κ���  dirNamebuf �� ����
		
		//���ϸ� ����
		strftime(tempbuf, sizeof(tempbuf), "%Y%m%d_%H%M%S.h264",tm);
		sprintf(fileNamebuf,"%s/%s",dirNamebuf,tempbuf);  // 20190316/20190316_1314.h264 ���丮��/���ϸ� <- ����

#ifdef DEBUG
	printf("dirNamebuf : %s \n",dirNamebuf);
	printf("filenamebuf : %s \n",fileNamebuf);
#endif
		
		struct dirent *de = NULL; // �ּҰ��� ������ NULL�� �ʱ�ȭ�Ͽ� ��������
		if((MP=dfopen())==NULL)
		{
			printf("Error: dfopen()\n");
			return -1;
		}
		dfget(MP);
		//�����ִ� ��ũ ������ 5%�̸��̶��
		//���� ������ ���丮�� �����Ѵ�
		remain = (((float)MP->size.avail/(float)MP->size.blocks) *100);
#ifdef DEBUG
				printf("blocks : %ld\n",MP->size.blocks); // �����üũ��
				printf("avail : %ld \n",MP->size.avail);  //��밡���� ���ũ��
				printf("remain(%):%f\n",remain);		  //��밡���� ��� �ۼ�������
#endif
		if(remain<5)
		{
			//TODO : ���� ������ ���丮�� ã�Ƽ� ���� �߰�

		}
		dfclose(MP);
		
		//���� �ð����� ���丮 ����
		//mkdir error ó�� (errno.h)
		//EACCES :�����Ϸ��� �θ� ���丮�� ��������� ����
		//EFAULT : pathname�� ���Ӱ����� �޸𸮰� �ƴ�
		//ENOTDIR : pathname�� ���丮�� �ƴ� ���� ����
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
		
		//�ڽ����μ��� ����
		pid =fork();

		//�θ� ���μ���
		if(pid>0)
		{
			//none
		}
		//�ڽ� ���μ���
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
