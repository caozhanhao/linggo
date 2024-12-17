#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FILENAME "api.py" 
#define VARNAME "temp_value_chat"
#define QUESTION "question.txt"

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
            sprintf(new_line, "%s = '''%s'''\n", varName, newValue);
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

char* gen_ai_quiz(const char* word)
{   
    char question[512] = "\0";

    strcpy(question, word);

    strcat(question,"，以该单词使用json格式输出一道大学英语选择题（仅有题目，abcd四个选项和答案，不包括任何代码和回答注释），json生成示例： { \"question\": \"<Question>\", \"options\": { \"A\": \"<Option A>\", \"B\": \"<Option B>\", \"C\": \"<Option C>\", \"D\": \"<Option D>\" }, \"answer\": \"<Correct Answer>\" }");
   
    modifyVariableInFile(FILENAME, VARNAME,question);
    system("python3 /home/zhuyin/api.py");  // 运行修改后的Python文件
    FILE *fp;
    fp = fopen(QUESTION, "r");
    if (fp== NULL) {
        perror("Error opening file");
        exit(1);
    }

    size_t bytesRead;
    static char content[512];
    bytesRead = fread(content,sizeof(char),sizeof(content)-1,fp);
    content[bytesRead] = '\0';
    fclose(fp);

    return content;
}

int main() {
    char *word = NULL;

    char *test = gen_ai_quiz("important");

    //printf("%s",test);

    return 0;
}