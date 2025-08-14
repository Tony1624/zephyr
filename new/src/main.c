#include<zephyr/kernel.h>
#include<zephyr/sys/printk.h>

K_SEM_DEFINE(SEM1,1,1);
K_SEM_DEFINE(SEM2,0,1);

void ping(){

    while (1)
    {

        k_sem_take(&SEM1, K_FOREVER);
        printk("ping\n");
        k_sem_give(&SEM2);

    }
    
}

void pong()
{
    while(1)
    {
        k_sem_take(&SEM2, K_FOREVER);
        printk("pong\n");
        k_sem_give(&SEM1);
    }
}

 K_THREAD_DEFINE(PING,1024,ping,NULL,NULL,NULL,1,0,0);
 K_THREAD_DEFINE(PONG,1024,pong,NULL,NULL,NULL,1,0,0);
 