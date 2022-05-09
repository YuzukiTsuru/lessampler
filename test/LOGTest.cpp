//
// Created by gloom on 2022/5/9.
//

#include "Utils/LOG.h"

class class_lift {
public:
    class_lift(){
        YALL_DEBUG_ << "Debug Message From Class";
    }
};

void func(){
    YALL_DEBUG_ << "Debug Message From Func";
}

int main() {
    YALL_DEBUG_.EnableDebug();
    YALL_DEBUG_ << "Debug Message 1";
    class_lift classLift;
    func();
    YALL_DEBUG_ << "Debug Message 2";
    YALL_DEBUG_.DisableDebug();
    YALL_DEBUG_ << "Disable Message 1";
    class_lift classLift1;
    func();
    YALL_INFO_ << "LOG Info";
    YALL_WARN_ << "LOG Warning";
    YALL_ERROR_ << "LOG Error";
    YALL_CRITICAL_ << "LOG Cirtical";
}
