#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
typedef struct PACKED         //定义一个cpu occupy的结构体
{

char name[20];      //定义一个char类型的数组名name有20个元素

unsigned int user; //定义一个无符号的int类型的user

unsigned int nice; //定义一个无符号的int类型的nice

unsigned int system;//定义一个无符号的int类型的system

unsigned int idle; //定义一个无符号的int类型的idle

}CPU_OCCUPY;

typedef struct PACKED1         //定义一个mem occupy的结构体
{

char name[20];               //定义一个char类型的数组名name有20个元素

unsigned long total;

char name2[20];

unsigned long free;

}MEM_OCCUPY;

/*获取内存使用率*/ 
int get_memoccupy (MEM_OCCUPY *mem) //对无类型get函数含有一个形参结构体类弄的指针O

{

    FILE *fd;

    int n;

    char buff[256];

    char buff1[256];

    char buff2[256];

    char buff3[256];

    char buff4[256];

    MEM_OCCUPY *m;

    m=mem;

    float mem_use;

    fd = fopen ("/proc/meminfo", "r");

    fgets (buff1, sizeof(buff1), fd);

    printf("%s\n",buff1);

    int num1 = atoi(buff1+15);

    printf("%d\n",num1);

    fgets (buff2, sizeof(buff2), fd);

    printf("%s\n",buff2);

    int num2 = atoi(buff2+15);

    fgets (buff3, sizeof(buff3), fd);

    printf("%s\n",buff3);

    int num3 = atoi(buff3+15);

    fgets (buff4, sizeof(buff4), fd);

    // sscanf (buff, "%s %u %s", m->name, &m->total, m->name2);

    printf("%s\n",buff4);

    int num4 = atoi(buff4+15);

    fgets (buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里

    printf("%s\n",buff);
    int num5 = atoi(buff+15);
    // 内存使用率(MEMUsedPerc)=100*(MemTotal-MemFree-Buffers-Cached)/MemTotal

    mem_use = (float)100*(num1-num2-num5-num4)/num1;

    printf("内存使用率为：%.2f%%\n",mem_use);

    fclose(fd);     //关闭文件fd

}

/*获取cpu使用率*/
 float cal_cpuoccupy (CPU_OCCUPY *o, CPU_OCCUPY *n)  
{      
    unsigned long od, nd;       
    unsigned long id, sd;     
    float cpu_use = 0;           
    od = (unsigned long) (o->user + o->nice + o->system +o->idle);//第一次(用户+优先级+系统+空闲)的时间再赋给od    
    nd = (unsigned long) (n->user + n->nice + n->system +n->idle);//第二次(用户+优先级+系统+空闲)的时间再赋给nd      
    id = (unsigned long) (n->user - o->user);    //用户第一次和第二次的时间之差再赋给id    
    sd = (unsigned long) (n->system - o->system);//系统第一次和第二次的时间之差再赋给sd  
    if((nd-od) != 0)
    {
         cpu_use = (float)((sd+id)*100)/(nd-od); //((用户+系统)乖100)除(第一次和第二次的时间差)再赋给g_cpu_used   
    }     
    else
    {   cpu_use = 0;          
       
    }   
    printf("cpu使用率为:%1.2f%%\n",cpu_use);     
    return cpu_use; 
}
void   get_cpuoccupy (CPU_OCCUPY *cpust) //对无类型get函数含有一个形参结构体类弄的指针O  
{       
        FILE *fd;            
        int n;              
        char buff[256];    
        CPU_OCCUPY *cpu_occupy;     
        cpu_occupy=cpust;           
        fd = fopen ("/proc/stat", "r");    
        fgets (buff, sizeof(buff), fd);          
        sscanf (buff, "%s %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle);   
        fclose(fd);     
 }   
            
 int main() 
 {     
    CPU_OCCUPY cpu_stat1;    
    CPU_OCCUPY cpu_stat2;    
    MEM_OCCUPY mem_stat;   
    float cpu;         
      //获取内存     
    get_memoccupy ((MEM_OCCUPY *)&mem_stat);         
      //第一次获取cpu使用情况   
    get_cpuoccupy((CPU_OCCUPY *)&cpu_stat1);   
    sleep(10);          
     //第二次获取cpu使用情况  
    get_cpuoccupy((CPU_OCCUPY *)&cpu_stat2);          
     //计算cpu使用率     
    cpu = cal_cpuoccupy ((CPU_OCCUPY *)&cpu_stat1, (CPU_OCCUPY *)&cpu_stat2);        
     return 0;  
}

