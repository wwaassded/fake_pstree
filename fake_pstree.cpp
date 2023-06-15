#include<malloc.h>
#include <cassert>
#include <cctype>
#include <cstring>
#include <string>
#include <dirent.h>
using namespace std;


struct commend_status {
    bool show_pids;
    bool numeric_sort;
    bool write_into_file;
    FILE* target_file;
};
commend_status status;

struct process{
    char process_name[20];
    char ppid[8];
    char pid[8];
    process *father;
    process *last_son;
    process *first_son;
    process *next_brother;
};

void printTree(process *root,bool flag);
void printTree(const string &prestr,process *node,bool flag);
void printTree(const string &prestr,process *node,bool flag,FILE *file) ;

int main(int argc, char *argv[]) {


    for (int i = 1; i < argc; i++) {
        if(strcmp(argv[i],"-p")==0 || strcmp(argv[i],"p")==0 || strcmp(argv[i],"--show-pids") != 0)
            status.show_pids = true;
        else  if(strcmp(argv[i],"-n")==0 || strcmp(argv[i],"n")==0 || strcmp(argv[i],"--numeric-sort") != 0)
            status.numeric_sort = true;
        else if(strcmp(argv[i],">")==0 || strcmp(argv[i],">>") != 0) {
            status.write_into_file = true;
            if(i == argc-1) {
                printf("you did not input the file ,so the result will not be written into file!\n");
                status.write_into_file = false;
                break;
            }
            else {
                if(argv[i][0] == '>')
                    status.target_file = fopen(argv[i+1],"w");
                else
                    status.target_file = fopen(argv[i+1],"a");
                if(status.target_file == nullptr) {
                    printf("can not open the file you want!\n");
                    return EXIT_FAILURE;
                }
                if(i != argc-2)
                    printf("you should input option before the >/>> and filename!\n the rest option will not be concidered!\n");
                break;
            }
        }
        else {
            printf("please input the correct option!\n");
            return EXIT_FAILURE;
        }
    }
    DIR *procdir = opendir("/proc");
    if(procdir == nullptr) {
        printf("pstree is not available for you!\n");
        return EXIT_FAILURE;
    }
    auto *root = (process*)malloc(sizeof(process));
    root->ppid[0] = '0';
    root->ppid[1] = '\0';
    root->pid[0] = '1';
    root->pid[1] = '\0';
    strcpy(root->process_name,"init(Ubuntu)");
    process *pre_process = root;
    struct dirent *ptr;
    ptr = readdir(procdir);
    while(ptr != nullptr){
        if(ptr->d_type == 4) {
            bool flag = true;
            for(int i=0; ptr->d_name[i]!='\0'; ++i)
                if(ptr->d_name[i]<'0' || ptr->d_name[i]>'9') {
                    flag = false;
                    break;
                }
            if(flag && strcmp(ptr->d_name,"1")!=0) {
                auto *tmp = (process*)malloc(sizeof(process));
                tmp->first_son = nullptr;
                strcpy(tmp->pid,ptr->d_name);
                char file_name[20];
                strcpy(file_name,"/proc/");
                strcat(file_name,ptr->d_name);
                strcat(file_name,"/status");
                FILE *status_file = fopen(file_name,"r");
                assert(status_file != nullptr);
                char detail[30];
                char *po_detail = nullptr;
                int line_number = 0;
                while(fgets(detail,30,status_file)) {
                    size_t len = strlen(detail);
                    detail[len-1] = '\0';
                    if(len <= 3)
                        continue;
                    po_detail = detail;
                    if(line_number == 0) {
                        while(isalpha(*po_detail) != 0) {po_detail = po_detail+1;}
                        while(isalpha(*po_detail) == 0) {po_detail = po_detail+1;}
                        strcpy(tmp->process_name,po_detail);
                    }
                    if(line_number == 6) {
                        while(isalpha(*po_detail)) {po_detail = po_detail +1 ;}
                        while(!isdigit(*po_detail)) {po_detail = po_detail+1;}
                        strcpy(tmp->ppid,po_detail);
                        break;
                    }
                    ++line_number;
                }
                if(pre_process != nullptr) {
                    while(pre_process!=nullptr && strcmp(tmp->ppid,pre_process->pid) != 0)
                        pre_process = pre_process->father;
                    if(pre_process!=nullptr && pre_process->first_son == nullptr) {
                        tmp->father = pre_process;
                        pre_process->first_son = tmp;
                        pre_process->last_son = tmp;
                        pre_process = tmp;
                    }
                    else if(pre_process != nullptr){
                        tmp->father = pre_process;
                        pre_process->last_son->next_brother = tmp;
                        pre_process->last_son = tmp;
                        pre_process = tmp;
                    }
                }
                if(status_file != nullptr)
                    fclose(status_file);
            }
        }
        ptr = readdir(procdir);
    }
    closedir(procdir);
    printTree(root,status.write_into_file);
    if(status.target_file != nullptr)
        fclose(status.target_file);
    return 0;
}

void printTree(process *root,bool flag) {
    if(!flag)
        printTree("",root,false);
    else
        printTree("",root,false,status.target_file);
}
void printTree(const string &prestr,process *node,bool flag) {
    printf("%s",prestr.c_str());
    if(flag)
        printf("|--");
    else if(node->father != nullptr)
        printf("|__");
    if(status.show_pids)
        printf("%s(%s)\n",node->process_name,node->pid);
    else
        printf("%s\n",node->process_name);
    for(process *tmp=node->first_son; tmp!=NULL; tmp=tmp->next_brother) {
        if(tmp->next_brother == nullptr)
            printTree(prestr+(flag?"|    ":"    "),tmp,false);
        else
            printTree(prestr+(flag?"|    ":"    "),tmp,true);
    }
    free(node);
}

void printTree(const string &prestr,process *node,bool flag,FILE *file) {
    if(file == nullptr) {
        printf("threre is something wrong!\n");
        return ;
    }
    else {
        fputs(prestr.c_str(),file);
        if(flag)
            fputs("|--",file);
        else if(node->father != nullptr)
            fputs("|__",file);
        if(status.show_pids) {
            printf("%s(%s)\n",node->process_name,node->pid);
            fputs(node->process_name,file);
            fputs(node->pid,file);
            fputs("\n",file);
        }
        else {
            fputs(node->process_name,file);
            fputs("\n",file);
        }
        for(process *tmp=node->first_son; tmp!=NULL; tmp=tmp->next_brother) {
            if(tmp->next_brother == nullptr)
                printTree(prestr+(flag?"|    ":"    "),tmp,false,file);
            else
                printTree(prestr+(flag?"|    ":"    "),tmp,true,file);
        }
    }
    free(node);
}
