//
//  main.cpp
//  nvram
//
//  Created by SpringHack on 15/12/14.
//  Copyright © 2015年 SpringHack. All rights reserved.
//

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void sigroutine(int dunno)
{
    printf("%s\n", "SpringHack");
    system("nvram -x -p > /Extra/nvram.plist");
    kill(getpid(), SIGKILL);
    return;
}

int main(int argc, char * argv[])
{
    system("nvram -x -f /Extra/nvram.plist");
    daemon(0, 0);
    for (int i=1;i<=31;++i)
        signal(i, sigroutine);
    for (;;) pause();
    return 0;
}