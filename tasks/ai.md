## AI 生成词汇题目

### 函数要求
```c
char* gen_ai_quiz(const char* word);
```

#### gen_ai_quiz 
- 接受一个 单词 用来传给 python，python 根据该单词生成 一个题目。  

其中，在 python 中解析大模型返回的 json， 重新生成符合 `给定格式的 json`，再写入 一个临时文件。 然后在 C语言 中读取该文件，返回文件中的字符串，该字符串即 `给定格式的 json`。

json格式如下所示

```json
{
  "question": "I am very ________ about the upcoming exam. I have been preparing for weeks.",
  "options": {
    "A": "nervous",
    "B": "excited",
    "C": "bored",
    "D": "tired"
  },
  "answer": "B"
}
```