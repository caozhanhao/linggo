#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 函数用于在文件中查找并修改变量
void modifyVariableInFile(const char *filename, const char *varName, const char *newValue) {
    FILE *fp_read, *fp_write;
    char buffer[1024];
    char new_line[1024];
    int found = 0;

    // 以只读模式打开Python文件
    fp_read = fopen(filename, "r");
    if (fp_read == NULL) {
        perror("Error opening file");
        exit(1);
    }

    // 创建临时文件用于写入修改后的内容
    fp_write = fopen("temp.py", "w");
    if (fp_write == NULL) {
        fclose(fp_read);
        perror("Error opening temp file for writing");
        exit(1);
    }

    // 逐行读取原文件内容，找到要修改的变量行并修改，其他行原样写入临时文件
    while (fgets(buffer, sizeof(buffer), fp_read)) {
        if (strstr(buffer, varName)!= NULL && found == 0) {
            // 构造新的包含变量修改后的行
            sprintf(new_line, "%s = '%s'\n", varName, newValue);
            fputs(new_line, fp_write);
            found = 1;
        } else {
            fputs(buffer, fp_write);
        }
    }

    fclose(fp_read);
    fclose(fp_write);

    if (!found) {
        remove("temp.py");  // 如果没找到变量，删除临时文件
        printf("Variable %s not found in %s.\n", varName, filename);
        exit(1);
    }

    // 删除原文件
    remove(filename);
    // 将临时文件重命名为原文件名，实现替换
    rename("temp.py", filename);
}

void select_func(char *newvalue)
{
   strcpy(newvalue, "请生成一道词汇填空类型英语选择题(A,B,C,D四个选项)，以json格式输出");  // 正确复制字符串
}

void chat_func(char *newvalue)
{   
    printf("您好，请问有什么能够帮助您？\n");
    scanf("%s", newvalue);
    strcat(newvalue,"(以json格式输出)");
}

int main() {
    char *newValue;
    newValue = (char *)malloc(1024 * sizeof(char));  //根据需要动态分配足够的内存空间

    if (newValue == NULL) {
        printf("内存分配失败\n");
        return 1;
    }

    // 指定要修改的Python文件和变量
    char *filename = "api.py";
    char *varName = "temp_value_chat";

    //聊天功能
    chat_func(newValue);
    //出题功能
    //select_test(newValue);

    // 调用函数修改变量
    modifyVariableInFile(filename, varName, newValue);
    free(newValue);  // 释放动态分配的内存

    system("python3 /home/zhuyin/api.py");  // 运行修改后的Python文件

    return 0;
}