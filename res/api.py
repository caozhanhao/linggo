from sparkai.llm.llm import ChatSparkLLM, ChunkPrintHandler
from sparkai.core.messages import ChatMessage
import json
import re
import os

# 星火认知大模型Spark Max的URL值，其他版本大模型URL值请前往文档（https://www.xfyun.cn/doc/spark/Web.html）查看
SPARKAI_URL = 'wss://spark-api.xf-yun.com/v4.0/chat'
# 星火认知大模型调用秘钥信息，请前往讯飞开放平台控制台（https://console.xfyun.cn/services/bm35）查看
SPARKAI_APP_ID = 'e9e7025f'
SPARKAI_API_SECRET = 'MGE4NTFiMjNhNDBkNGMzODhiZjNjMzIz'
SPARKAI_API_KEY = '47ab4003f720eb8926ef1f0aaddc4740'
# 星火认知大模型Spark Max的domain值，其他版本大模型domain值请前往文档（https://www.xfyun.cn/doc/spark/Web.html）查看
SPARKAI_DOMAIN = '4.0Ultra'

temp_value_chat = '''thinking，以该单词使用json格式输出一道大学英语选择题（仅有题目，abcd四个选项和答案，不包括任何代码和回答注释），json生成示例： { "question": "<Question>", "options": { "A": "<Option A>", "B": "<Option B>", "C": "<Option C>", "D": "<Option D>" }, "answer": "<Correct Answer>" }'''


# json格式判断函数，判断输出的json的键值是否包含'question'、'options'和'answer'以及'options'是否包含'A'、'B'、'C'、'D'四个选项
def validate_json_format(data):
    """
    验证 JSON 格式是否符合要求：必须包含 'question'、'options' 和 'answer'。
    """
    try:
        json_data = json.loads(data)
        if 'question' in json_data and 'options' in json_data and 'answer' in json_data:
            # 检查 options 是否包含 A、B、C、D 四个选项
            options = json_data.get('options', {})
            if all(k in options for k in ['A', 'B', 'C', 'D']):
                return True
        return False
    except json.JSONDecodeError:
        return False


if __name__ == '__main__':
    spark = ChatSparkLLM(
        spark_api_url=SPARKAI_URL,
        spark_app_id=SPARKAI_APP_ID,
        spark_api_key=SPARKAI_API_KEY,
        spark_api_secret=SPARKAI_API_SECRET,
        spark_llm_domain=SPARKAI_DOMAIN,
        streaming=False,
    )

    messages = [ChatMessage(
        role="user",
        content=temp_value_chat
    )]

    handler = ChunkPrintHandler()
    a = spark.generate([messages])

    # 从 result.generations[0][0].text 提取文本
    try:
        generated_reply = a.generations[0][0].text
    except (IndexError, AttributeError):
        generated_reply = "请你重试一下"

    # 去掉 JSON 格式的代码块标记（前后的```以及```json）
    cleaned_reply = re.sub(r'```json|```', '', generated_reply).strip()

    # 判断json格式是否正确，如果格式不正确，重新生成
    if not validate_json_format(cleaned_reply):
        # print("正在重新生成...")
        a = spark.generate([messages])
        try:
            generated_reply = a.generations[0][0].text
        except (IndexError, AttributeError):
            generated_reply = "请你重试一下"
        cleaned_reply = re.sub(r'```json|```', '', generated_reply).strip()
        # 如果重新生成的仍不正确，退出
        if not validate_json_format(cleaned_reply):
            cleaned_reply = "请你重试一下，如果问题依然存在，请联系客服"
            exit()

    # 获取当前py文件所在的目录
    current_directory = os.path.dirname(os.path.abspath(__file__))
    # 定义文件名
    file_name = "question.txt"
    # 定义文件路径
    file_path = os.path.join(current_directory, file_name)
    # 写入json文件（写入前会先清空文件内容）
    with open(file_path, 'w', encoding='utf-8') as temp_file:
        json.dump(json.loads(cleaned_reply), temp_file, ensure_ascii=False, indent=4)
