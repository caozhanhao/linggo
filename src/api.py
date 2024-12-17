from sparkai.llm.llm import ChatSparkLLM, ChunkPrintHandler
from sparkai.core.messages import ChatMessage

#星火认知大模型Spark Max的URL值，其他版本大模型URL值请前往文档（https://www.xfyun.cn/doc/spark/Web.html）查看
SPARKAI_URL = 'wss://spark-api.xf-yun.com/v4.0/chat'
#星火认知大模型调用秘钥信息，请前往讯飞开放平台控制台（https://console.xfyun.cn/services/bm35）查看
SPARKAI_APP_ID = 'e9e7025f'
SPARKAI_API_SECRET = 'MGE4NTFiMjNhNDBkNGMzODhiZjNjMzIz'
SPARKAI_API_KEY = '47ab4003f720eb8926ef1f0aaddc4740'
#星火认知大模型Spark Max的domain值，其他版本大模型domain值请前往文档（https://www.xfyun.cn/doc/spark/Web.html）查看
SPARKAI_DOMAIN = '4.0Ultra'

temp_value_chat = '你好(以json格式输出)'

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
        content= temp_value_chat
    )]
    handler = ChunkPrintHandler()
    a = spark.generate([messages])
    #print(a)

    try:
        generated_reply = a.generations[0][0].text
    except(IndexError, AttributeError):
        generated_reply = "Sorry, I don't understand."

    print(generated_reply)