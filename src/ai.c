#include "linggo/ai.h"
#include "linggo/server.h"
#include "linggo/utils.h"
#include "json-parser/json.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FILENAME "api.py"
#define VARNAME "temp_value_chat"
#define QUESTION "question.txt"

// 函数用于在文件中查找并修改变量
void modifyVariableInFile(const char* filename, const char* varName, const char* newValue) {
    FILE *fp_read, *fp_write;
    char buffer[1024];
    char new_line[1024];
    int found = 0;

    // 以只读模式打开Python文件
    fp_read = fopen(filename, "r");
    if (fp_read == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    // 创建临时文件用于写入修改后的内容
    fp_write = fopen("temp.py", "w");
    if (fp_write == NULL)
    {
        fclose(fp_read);
        perror("Error opening temp file for writing");
        exit(1);
    }

    // 逐行读取原文件内容，找到要修改的变量行并修改，其他行原样写入临时文件
    while (fgets(buffer, sizeof(buffer), fp_read))
    {
        if (strstr(buffer, varName) != NULL && found == 0)
        {
            // 构造新的包含变量修改后的行
            sprintf(new_line, "%s = '''%s'''\n", varName, newValue);
            fputs(new_line, fp_write);
            found = 1;
        }
        else
        {
            fputs(buffer, fp_write);
        }
    }

    fclose(fp_read);
    fclose(fp_write);

    if (!found)
    {
        remove("temp.py"); // 如果没找到变量，删除临时文件
        printf("Variable %s not found in %s.\n", varName, filename);
        exit(1);
    }

    // 删除原文件
    remove(filename);
    // 将临时文件重命名为原文件名，实现替换
    rename("temp.py", filename);
}

enum LINGGO_CODE linggo_ai_gen_quiz(const char* word, linggo_ai_quiz* quiz) {
    char question[512] = "\0";

    char filename[64] = "\0";
    strcat(filename, linggo_svrctx.resource_path);
    strcat(filename, "/");
    strcat(filename, FILENAME);

    strcpy(question, word);

    strcat(question,
           "，以该单词使用json格式输出一道大学英语选择题（仅有题目，abcd四个选项和答案，不包括任何代码和回答注释），json生成示例： { \"question\": \"<Question>\", \"options\": { \"A\": \"<Option A>\", \"B\": \"<Option B>\", \"C\": \"<Option C>\", \"D\": \"<Option D>\" }, \"answer\": \"<Correct Answer>\" }");

    modifyVariableInFile(filename, VARNAME, question);

    char command[64] = "";
    strcat(command, linggo_svrctx.python);
    strcat(command, " ");
    strcat(command, filename);
    system(command); // 运行修改后的Python文件


    char qfilename[64] = "\0";
    strcat(qfilename, linggo_svrctx.resource_path);
    strcat(qfilename, "/");
    strcat(qfilename, QUESTION);

    FILE* fp = fopen(qfilename, "r");
    if (fp == NULL)
        return LINGGO_AIGEN_QUIZ_ERROR;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buffer = malloc(size + 1);
    if (buffer == NULL) return LINGGO_OUT_OF_MEMORY;
    fread(buffer, 1, size, fp);
    fclose(fp);
    buffer[size] = '\0';

    json_value* json = json_parse(buffer, size);
    if (json == NULL || json->type != json_object)
        return LINGGO_AIGEN_QUIZ_ERROR;

    json_value* jquestion = linggo_json_find_key(json, "question");
    json_value* janswer = linggo_json_find_key(json, "answer");
    json_value* joptions = linggo_json_find_key(json, "options");

    if (joptions == NULL || jquestion == NULL || janswer == NULL
        || joptions->type != json_object
        || janswer->type != json_string
        || jquestion->type != json_string
        || janswer->u.string.length != 1
        || (janswer->u.string.ptr[0] != 'A' && janswer->u.string.ptr[0] != 'B'
            && janswer->u.string.ptr[0] != 'C' && janswer->u.string.ptr[0] != 'D'))
        return LINGGO_AIGEN_QUIZ_ERROR;

    json_value* ja = linggo_json_find_key(joptions, "A");
    json_value* jb = linggo_json_find_key(joptions, "B");
    json_value* jc = linggo_json_find_key(joptions, "C");
    json_value* jd = linggo_json_find_key(joptions, "D");

    if (ja == NULL || jb == NULL || jc == NULL || jd == NULL
        || ja->type != json_string || jb->type != json_string
        || jc->type != json_string || jd->type != json_string)
        return LINGGO_AIGEN_QUIZ_ERROR;

    quiz->question = malloc(jquestion->u.string.length + 1);
    strcpy(quiz->question, jquestion->u.string.ptr);
    quiz->options[0] = malloc(ja->u.string.length + 1);
    strcpy(quiz->options[0], ja->u.string.ptr);
    quiz->options[1] = malloc(jb->u.string.length + 1);
    strcpy(quiz->options[1], jb->u.string.ptr);
    quiz->options[2] = malloc(jc->u.string.length + 1);
    strcpy(quiz->options[2], jc->u.string.ptr);
    quiz->options[3] = malloc(jd->u.string.length + 1);
    strcpy(quiz->options[3], jd->u.string.ptr);
    quiz->answer = janswer->u.string.ptr[0] - 'A';

    if (quiz->answer > 3)
        return LINGGO_AIGEN_QUIZ_ERROR;

    free(buffer);
    return LINGGO_OK;
}

void linggo_ai_quiz_free(linggo_ai_quiz quiz) {
    free(quiz.question);
    free(quiz.options[1]);
    free(quiz.options[2]);
    free(quiz.options[3]);
}
