#include "Device.h"
#include <iostream>

int main(){
  
  ChimeraTK::Device d("(xdma:xdma)");
  d.open();
  std::cout << "bar 0 address 0 is " << d.read<int>("/#/0/0*4") << std::endl;
}
