#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "ethercat.h"
#define EC_TIMEOUTMON 500

#include <iostream>
#include <bitset>
#include <chrono> 

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <sstream>
#include <csignal>
#include <bitset>
#include <inttypes.h>

typedef uint32_t uint4;
using namespace std;

char IOmap[4096];
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;
int clientSocket;

void io()
{
  for ( int i=0; i <=100; i=i+1)
      std::cout << "Inputs -" << i <<"  :  " << *(uint16*)(ec_slave[0].inputs + i)  << std::endl;
  std::cout << "\n";
  for ( int i=0; i <=100; i=i+1)
      std::cout << "Outputs -"<< i <<"  :  " << *(uint16*)(ec_slave[0].outputs + i)  << std::endl;
  std::cout << "\n";
  
}

void ethercat_test(char *ifname)
{   
    inOP = FALSE;
    std::cout << "\033[1;34m Starting EtherCAT test program \033[0m\n";
   
    if (ec_init(ifname))
    {
      std::cout << "\033[0;32m EtherCAT init succeeded on \033[0m" << ifname <<"\n";
      
      /* find ,auto-config and wait for all slaves to reach Operational state*/ 
      // exactly same as simple_test.c
      
      if ( ec_config_init(FALSE) > 0 )
	{ 
	  std::cout << "\033[0;33m slaves found and configured : \033[0m" << ec_slavecount <<"\n";
	  
	  ec_config_map(&IOmap);
	  ec_configdc();
	  ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 4);
	  printf("segments : %d : %d %d %d %d\n",ec_group[0].nsegments ,ec_group[0].IOsegment[0],ec_group[0].IOsegment[1],ec_group[0].IOsegment[2],ec_group[0].IOsegment[3]);
	  expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
	  ec_slave[0].state = EC_STATE_OPERATIONAL;
	  ec_send_processdata();// send one valid process data to make outputs in slaves happy
	  ec_receive_processdata(EC_TIMEOUTRET);
	  ec_writestate(0);// request OP state for all slaves
	  int chk = 40;
	  
	  do // wait for all slaves to reach OP state 
	  {
	    ec_send_processdata();
	    ec_receive_processdata(EC_TIMEOUTRET);
	    ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
	  }
	  while (chk-- && (ec_slave[1].state != EC_STATE_OPERATIONAL));

	  
	  /* working on the slaves */
	  
	  if (ec_slave[0].state == EC_STATE_OPERATIONAL )
	  {
	    std::cout << "\033[0;32m Operational state reached for slaves\033[0m \n";	    
	    inOP = TRUE;
// 	    std::bitset<4>  one(1);
	    while (true)
	      {
// 		*(ec_slave[0].outputs) = 230; // beckhoff slave 1 8ch DO 8 bits boolean- 230 = 11100110 (o/p through 5 channels)
// 		*(ec_slave[0].outputs + 1) = 252; // beckhoff slave 2 8ch DO 8 bits boolean - 252 = 11111100 (o/p through 6 channels)
//  		*(ec_slave[0].outputs) = 1;
// 		*(ec_slave[0].outputs) = 12;
		
		ec_send_processdata();
		wkc = ec_receive_processdata(EC_TIMEOUTRET);
 		io();
		osal_usleep(5000);
	      }
	      inOP = FALSE;
	    }
	  else
	  {
	    printf("Not all slaves reached operational state.\n");
	    ec_readstate();
	    for(int i = 1; i<=ec_slavecount ; i++)
	    {
		if(ec_slave[i].state != EC_STATE_OPERATIONAL)
		{
		    printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
			i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
		}
	    }
	  }
	  
	  printf("\nRequest init state for all slaves\n");
	  ec_slave[0].state = EC_STATE_INIT;
	  ec_writestate(0);
	}
      else
	  std::cout << "\033[1;31m No Slave found \033[0m\n";
	
      std::cout << "\033[1;33m Close socket\033[0m\n";
      close(clientSocket);
      ec_close();
    }
    else
	std::cout << "\033[0;31m No socket connection on : \033[0m" << ifname << "\033[0;32m [hint : execute as root - $sudo ...]\033[0m\n";
}

OSAL_THREAD_FUNC ecatcheck( void *ptr )		// exactly same as simple_test.c
{
    int slave;
    (void)ptr;                  /* Not used */

    while(1)
    {
        if( inOP && ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate))
        {
            if (needlf)
            {
               needlf = FALSE;
               printf("\n");
            }
            /* one ore more slaves are not responding */
            ec_group[currentgroup].docheckstate = FALSE;
            ec_readstate();
            for (slave = 1; slave <= ec_slavecount; slave++)
            {
               if ((ec_slave[slave].group == currentgroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
               {
                  ec_group[currentgroup].docheckstate = TRUE;
                  if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                  {
                     printf("ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                     ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state == EC_STATE_SAFE_OP)
                  {
                     printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                     ec_slave[slave].state = EC_STATE_OPERATIONAL;
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state > EC_STATE_NONE)
                  {
                     if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d reconfigured\n",slave);
                     }
                  }
                  else if(!ec_slave[slave].islost)
                  {
                     /* re-check state */
                     ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                     if (ec_slave[slave].state == EC_STATE_NONE)
                     {
                        ec_slave[slave].islost = TRUE;
                        printf("ERROR : slave %d lost\n",slave);
                     }
                  }
               }
               if (ec_slave[slave].islost)
               {
                  if(ec_slave[slave].state == EC_STATE_NONE)
                  {
                     if (ec_recover_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d recovered\n",slave);
                     }
                  }
                  else
                  {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d found\n",slave);
                  }
               }
            }
            if(!ec_group[currentgroup].docheckstate)
               printf("OK : all slaves resumed OPERATIONAL.\n");
        }
        osal_usleep(10000);
    }
}

int main(int argc, char *argv[])
{
   std::cout << "\033[1;34m SOEM (Simple Open EtherCAT Master) - PWM test \033[0m\n";
   if (argc > 1)
   {
      osal_thread_create(&thread1, 128000, &ecatcheck, (void*) &ctime);
      ethercat_test(argv[1]);
   }
   else
      std::cout << "\033[0;31m Please provide the required argument \033[0m \033[0;32m [hint : $ifconfig - Ethernet]\033[0m\n";

   std::cout << "\033[1;33m End Program \033[0m\n";
   return (0);
}
