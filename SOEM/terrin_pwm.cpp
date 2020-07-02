/** \file
 * \brief Example code for Simple Open EtherCAT master
 *
 * Usage : simple_test [ifname1]
 * ifname is NIC interface, f.e. eth0
 *
 * This is a minimal test.
 *
 * (c)Arthur Ketels 2010 - 2011
 */

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

using namespace std;

char IOmap[4096];
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;
int clientSocket;

void signalHandler( int signum ) {
   cout << "Interrupt signal (" << signum << ") received.\n"; 
   close(clientSocket);
   exit(signum);  
}

void pwm_io()
{
  std::cout << "PWM Inputs" << std::endl ;
  std::cout << " Diagnostc         :  " << *(uint16*)(ec_slave[0].inputs + 2)  << std::endl;
  std::cout << " Channel 1, Control:  " << *(uint16*)(ec_slave[0].inputs + 4)  << std::endl;
  std::cout << " Channel 1, Word   :  " << *(uint16*)(ec_slave[0].inputs + 6)  << std::endl;
  std::cout << " Channel 2, Control:  " << *(uint16*)(ec_slave[0].inputs + 8)  << std::endl;
  std::cout << " Channel 2, Word   :  " << *(uint16*)(ec_slave[0].inputs + 10) << std::endl;
  
  std::cout << "PWM Outputs" << std::endl ;
  std::cout << " Diagnostc         :  " << *(uint16*)(ec_slave[0].outputs + 2)   << std::endl;
  std::cout << " Channel 1, Status :  " << *(uint16*)(ec_slave[0].outputs + 4)   << std::endl;
  std::cout << " Channel 1, Word   :  " << *(uint16*)(ec_slave[0].outputs + 6)   << std::endl;
  std::cout << " Channel 2, Status :  " << *(uint16*)(ec_slave[0].outputs + 8)   << std::endl;
  std::cout << " Channel 2, Word   :  " << *(uint16*)(ec_slave[0].outputs + 10)  << std::endl;
}

void pwm_test(char *ifname)
{
    //inputs
    float spot_len_limit = 100;
    uint16 freq_low = 1000;
    uint16 duty_low = 8160; // 100% = 32736, 50% = 16352, 25% = 8160, 0.2% = 64, 0.1% = 32
    uint16 freq_high = 5000;
    uint16 duty_high = 16352; // 100% = 32736, 50% = 16352, 25% = 8160, 0.2% = 64, 0.1% = 32
    int port = 5000;
    
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_aton("127.0.0.1", &hint.sin_addr);

    int listening = socket(AF_INET, SOCK_STREAM, 0);
    bind(listening, (sockaddr*)&hint, sizeof(hint));
    listen(listening, SOMAXCONN);

    sockaddr_in client; 
    socklen_t clientSize = sizeof(client);
    clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
//     if(clientSocket<=0)

    char host[NI_MAXHOST];      
    char service[NI_MAXSERV];   
    memset(host, 0, NI_MAXHOST); 
    memset(service, 0, NI_MAXSERV);

    cout << "\033[1;32m 127.0.0.1 connected on port \033[0m" << port << endl;

    close(listening); 
    char buf[4096];
    
    inOP = FALSE;
    std::cout << "\033[1;34m Starting PWM test program \033[0m\n";
   
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
	  while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

	  
	  /* working on the slaves */
	  
	  if (ec_slave[0].state == EC_STATE_OPERATIONAL )
	  {
	    printf("Operational state reached for all slaves.\n");
	    
	    inOP = TRUE;
	    uint64 status_1 = 0;
	    uint64 status_2 = 0;
	    
	    // PWM has 5 datas (2 bytes each) as output : 1. Diagnostc, 2.Channel1 Control, 3.Channel1 Word, 4.Channel2 Control, 5.Channel2 Word
	    // PWM has 5 datas (2 bytes each) as input  : 1. Diagnostc, 2.Channel1 Status,  3.Channel1 Word, 4.Channel2 Status,  5.Channel2 Word
	    
	    while (true)
	      {
		memset(buf, 0, 4096);

		int bytesReceived = recv(clientSocket, buf, 4096, 0); // Wait for client to send data
		if (bytesReceived == -1){cerr << "Error in recv(). Quitting" << endl;break;}
		if (bytesReceived == 0){ cout << "Client disconnected " << endl; break;}

		std::string spot_len_s = string(buf, 0, bytesReceived) ;
		stringstream ss; float spot_len_fl;
		ss << spot_len_s; 
		ss >> spot_len_fl;
		
		if (spot_len_fl < spot_len_limit)
		{
		    status_2 = 0;
		    while(status_1 != 131)  //loops for few times (3-4 times) // staus becomes to 131 (10000011) after freq set
		    {
		      *(ec_slave[0].outputs + 4) = 195; // set control to 195 (11000011) to write the frequency value (see catlog)
		      *(uint16*)(ec_slave[0].outputs + 6) = freq_low;
		      ec_send_processdata();
		      ec_receive_processdata(EC_TIMEOUTRET);
		      status_1 = (*(ec_slave[0].inputs + 4));
		      osal_usleep(5000);
		    }

		    *(ec_slave[0].outputs + 4) = 0;
		    *(uint16*)(ec_slave[0].outputs + 6) = duty_low ;

		    ec_send_processdata();
		    wkc = ec_receive_processdata(EC_TIMEOUTRET);
		}
		
		if (spot_len_fl > spot_len_limit)
		{
		    status_1 = 0;
		    while(status_2 != 131)  //loops for few times (3-4 times)
		    {
		      *(ec_slave[0].outputs + 4) = 195; // set control to 195 (11000011) to write the frequency value (see catlog)
		      *(uint16*)(ec_slave[0].outputs + 6) = freq_high;
		      ec_send_processdata();
		      ec_receive_processdata(EC_TIMEOUTRET);
		      status_2 = (*(ec_slave[0].inputs + 4));
		      osal_usleep(5000);
		    }

		    *(ec_slave[0].outputs + 4) = 0;
		    *(uint16*)(ec_slave[0].outputs + 6) = duty_high ;

		    ec_send_processdata();
		    wkc = ec_receive_processdata(EC_TIMEOUTRET);
		}
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
   signal(SIGINT, signalHandler); 
   std::cout << "\033[1;34m SOEM (Simple Open EtherCAT Master) - PWM test \033[0m\n";
   if (argc > 1)
   {
      osal_thread_create(&thread1, 128000, &ecatcheck, (void*) &ctime);
      pwm_test(argv[1]);
   }
   else
      std::cout << "\033[0;31m Please provide the required argument \033[0m \033[0;32m [hint : $ifconfig - Ethernet]\033[0m\n";

   std::cout << "\033[1;33m End Program \033[0m\n";
   return (0);
}
