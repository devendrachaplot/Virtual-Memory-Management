#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    PrintString("Ready to greet \n");
    int sz=4096;
    char arr[4096];
    int i;
    for(i=0;i<sz;i++){
        //PrintInt(i);
        //PrintString("");
        arr[i]='a';
    }
    int j;
    PrintString("done putting\n");
    for(i=sz-10;i<sz;i++){
        PrintInt(arr[i]);
        PrintString("");
    }
    PrintString("done greeting \n");
    return 0;
}

// int
// main(int argc, char *argv[])
// {
//     PrintString("Helloworld called\n");
//     int x=0;
// 	int a[100000000],i;
// 	// x = helloworld();
// 	for(i=0;i<100000000;i+=1){
//         // PrintInt(i);
//         // PrintString("\n",1);
//         a[i] = i;
// 		x+=a[i];
// 	}
// 	PrintInt(1);
//     return 0;
// }
