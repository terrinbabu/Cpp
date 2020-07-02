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
#include <iostream>
#include "ethercat.h"
#include <bitset>
#include <chrono> 
#define EC_TIMEOUTMON 500

char IOmap[4096];
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;

void setFreqPWM()
{
  uint64 status = 0;
  std::bitset<8>  writeFreqCommand(195);
  std::bitset<16> freqVal(5000);
  uint16 dutyCh1 = 8160;
  uint16 dutyCh2 = 16352;
  std::bitset<8>  empty8(0);
  
  while(status != 131)
  {
    *(ec_slave[0].outputs + 4) = (uint8)(writeFreqCommand.to_ulong());
    *(uint16*)(ec_slave[0].outputs + 6) = (uint16)(freqVal.to_ulong());
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    status = (*(ec_slave[0].inputs + 4));
    osal_usleep(5000);
  }

  *(uint16*)(ec_slave[0].outputs + 6) = dutyCh1 ;
  *(uint16*)(ec_slave[0].outputs +10) = dutyCh2 ;
  
  *(ec_slave[0].outputs + 4) = (uint8)(empty8.to_ulong());
  
  ec_send_processdata();
}

void simpletest(char *ifname)
{
    int i, j, oloop, iloop, chk;
    needlf = FALSE;
    inOP = FALSE;

   printf("Starting simple test\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname))
   {
      printf("ec_init on %s succeeded.\n",ifname);
      /* find and auto-config slaves */


       if ( ec_config_init(FALSE) > 0 )
      {
         printf("%d slaves found and configured.\n",ec_slavecount);

         ec_config_map(&IOmap);

         ec_configdc();

         printf("Slaves mapped, state to SAFE_OP.\n");
         /* wait for all slaves to reach SAFE_OP state */
         ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 4);
         
         oloop = 12;
         iloop = 28;
         printf("segments : %d : %d %d %d %d\n",ec_group[0].nsegments ,ec_group[0].IOsegment[0],ec_group[0].IOsegment[1],ec_group[0].IOsegment[2],ec_group[0].IOsegment[3]);

         printf("Request operational state for all slaves\n");
         expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
         printf("Calculated workcounter %d\n", expectedWKC);
         ec_slave[0].state = EC_STATE_OPERATIONAL;
         /* send one valid process data to make outputs in slaves happy*/
         ec_send_processdata();
         ec_receive_processdata(EC_TIMEOUTRET);
         /* request OP state for all slaves */
         ec_writestate(0);
         chk = 40;
         /* wait for all slaves to reach OP state */
         do
         {
            ec_send_processdata();
            ec_receive_processdata(EC_TIMEOUTRET);
            ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
         }
         while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
         if (ec_slave[0].state == EC_STATE_OPERATIONAL )
         {
            printf("Operational state reached for all slaves.\n");
            inOP = TRUE;
            /* cyclic loop */
            
            setFreqPWM();
            
            for(i = 1; i <= 100000; i++)
            {
               ec_send_processdata();
               wkc = ec_receive_processdata(EC_TIMEOUTRET);

                    if(wkc >= expectedWKC)
                    {
                      auto start = std::chrono::high_resolution_clock::now();
                      printf("Processdata cycle %4d, WKC %d , O:", i, wkc);
                      printf(" Outputs\n:");
                      std::cout << std::endl;
                      std::cout << " Diagnostc :  " << std::bitset<16>(*(uint16*)(ec_slave[0].outputs + 2)) << std::endl;
                      std::cout << " Channel 1 Control :  " << std::bitset<8>(*(ec_slave[0].outputs + 4)) << std::endl;
                      std::cout << " Channel 1, Word 1 :  " << std::bitset<16>(*(uint16*)(ec_slave[0].outputs + 6)) << std::endl;
                      std::cout << " Channel 2 Control :  " << std::bitset<8>(*(ec_slave[0].outputs + 8)) << std::endl;
                      std::cout << " Channel 2, Word 2 :  " << std::bitset<16>(*(uint16*)(ec_slave[0].outputs + 10)) << std::endl;
                      
                      printf(" Inputs\n:");
                      std::cout << std::endl;
                      std::cout << " Channel 1 status :  " << std::bitset<8>(*(ec_slave[0].inputs + 4)) << std::endl;
                      std::cout << " Channel 1, Word 1 :  " << std::bitset<16>(*(uint16*)(ec_slave[0].inputs + 6)) << std::endl;
                      std::cout << " Channel 2 status :  " << std::bitset<8>(*(ec_slave[0].inputs + 8)) << std::endl;
                      std::cout << " Channel 2, Word 2 :  " << std::bitset<16>(*(uint16*)(ec_slave[0].inputs + 10)) << std::endl;

                      std::cout << " AI 1 :  " << (*(uint16*)(ec_slave[0].inputs + 12)) << std::endl;
                      std::cout << " AI 2 :  " << (*(uint16*)(ec_slave[0].inputs + 14)) << std::endl;
                      std::cout << " AI 3 :  " << (*(uint16*)(ec_slave[0].inputs + 16)) << std::endl;
                      std::cout << " AI 4 :  " << (*(uint16*)(ec_slave[0].inputs + 18)) << std::endl;
                      std::cout << " AI 5 :  " << (*(uint16*)(ec_slave[0].inputs + 20)) << std::endl;
                      std::cout << " AI 6 :  " << (*(uint16*)(ec_slave[0].inputs + 22)) << std::endl;
                      std::cout << " AI 7 :  " << (*(uint16*)(ec_slave[0].inputs + 24)) << std::endl;
                      std::cout << " AI 8 :  " << (((*(uint16*)(ec_slave[0].inputs + 26)))/3276.0) << std::endl;
                      
                      printf(" T:%"PRId64"\r",ec_DCtime);
                      needlf = TRUE;
                      auto finish = std::chrono::high_resolution_clock::now();
                      std::chrono::duration<double> elapsed = finish - start;
                      std::cout << "Frequency HZ: " << (1/(elapsed.count())) << std::endl;
                    }
                    osal_usleep(5000);

                }
                inOP = FALSE;
            }
            else
            {
                printf("Not all slaves reached operational state.\n");
                ec_readstate();
                for(i = 1; i<=ec_slavecount ; i++)
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
            /* request INIT state for all slaves */
            ec_writestate(0);
        }
        else
        {
            printf("No slaves found!\n");
        }
        printf("End simple test, close socket\n");
        /* stop SOEM, close socket */
        ec_close();
    }
    else
    {
        printf("No socket connection on %s\nExcecute as root\n",ifname);
    }
}

OSAL_THREAD_FUNC ecatcheck( void *ptr )
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
   printf("SOEM (Simple Open EtherCAT Master)\nSimple test\n");

   if (argc > 1)
   {
      /* create thread to handle slave error handling in OP */
//      pthread_create( &thread1, NULL, (void *) &ecatcheck, (void*) &ctime);
      osal_thread_create(&thread1, 128000, &ecatcheck, (void*) &ctime);
      /* start cyclic part */
      simpletest(argv[1]);
   }
   else
   {
      printf("Usage: simple_test ifname1\nifname = eth0 for example\n");
   }

   printf("End program\n");
   return (0);
}
