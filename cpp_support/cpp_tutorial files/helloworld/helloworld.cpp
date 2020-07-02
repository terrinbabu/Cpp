#include <iostream>
#include <bitset>
using namespace std;

int main()
{
  cout << "Hello Linux!!\n";
  
  cout << bitset<8>(195) <<endl; //binary value of the number 
  cout << bitset<4>(10) <<endl; //binary value of the number 
  int my_num = 195;
  cout << bitset<8>(my_num) <<endl;
  
  bitset<8> my_bit;
  my_bit = my_num;
  cout << my_bit << endl;
  
  std::bitset<8>  my_bit_val(195);
  cout << my_bit_val <<endl;
  
  cout << my_bit_val.to_ulong() << endl;
  
  
  int my_var = 25;
  int *add = &my_var;
  cout << add << endl;
  int de_ref = *add;
  cout << de_ref << endl;
  
  return 0;
}