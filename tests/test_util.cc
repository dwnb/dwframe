#include "dwframe/dwframe.h"
#include "assert.h"
#include "dwframe/fiber.h"
void test_util(){
    dwframe_LOG_INFO(dwframe_log_name("system"))<<"\n"<<dwframe::BacktraceToString(10,0,"\t");
}
int main(){
    test_util();
    return 0;
}