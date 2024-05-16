#include <string.h>
#include <unistd.h>
#include <string>
#include "rkllm.h"
#include <fstream>
#include <iostream>
#include <csignal>
#include <vector>
#include "WebsocketServer.h"
#include <thread>
#include <asio/io_service.hpp>

//The port number the WebSocket server listens on
#define PORT_NUMBER 8089
#define PROMPT_TEXT_PREFIX "<|im_start|>system You are a helpful assistant. <|im_end|> <|im_start|>user"
#define PROMPT_TEXT_POSTFIX "<|im_end|><|im_start|>assistant"

using namespace std;
LLMHandle llmHandle = nullptr;

void exit_handler(int signal)
{
    if (llmHandle != nullptr)
    {
        {
            cout << "程序即将退出" << endl;
            LLMHandle _tmp = llmHandle;
            llmHandle = nullptr;
            rkllm_destroy(_tmp);
        }
        exit(signal);
    }
}


void callback(const char *text, void *userdata, LLMCallState state)
{

    if (state == LLM_RUN_FINISH)
    {
        printf("\n");
    }
    else if (state == LLM_RUN_ERROR)
    {
        printf("\\run error\n");
    }
    else{
        printf("%s", text);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage:%s [rkllm_model_path]\n", argv[0]);
        return -1;
    }
    signal(SIGINT, exit_handler);
    string rkllm_model(argv[1]);
    printf("rkllm init start\n");

    //设置参数及初始化
    RKLLMParam param = rkllm_createDefaultParam();
    param.modelPath = rkllm_model.c_str();
    param.target_platform = "rk3588";
    param.num_npu_core = 2;
    param.top_k = 1;
    param.max_new_tokens = 256;
    param.max_context_len = 512;
    rkllm_init(&llmHandle, param, callback);
    printf("rkllm init success\n");

    vector<string> pre_input;
    pre_input.push_back("把下面的现代文翻译成文言文：到了春风和煦，阳光明媚的时候，湖面平静，没有惊涛骇浪，天色湖光相连，一片碧绿，广阔无际；沙洲上的鸥鸟，时而飞翔，时而停歇，美丽的鱼游来游去，岸上与小洲上的花草，青翠欲滴。");
    pre_input.push_back("以咏梅为题目，帮我写一首古诗，要求包含梅花、白雪等元素。");
    pre_input.push_back("上联: 江边惯看千帆过");
    pre_input.push_back("把这句话翻译成中文：Knowledge can be acquired from many sources. These include books, teachers and practical experience, and each has its own advantages. The knowledge we gain from books and formal education enables us to learn about things that we have no opportunity to experience in daily life. We can also develop our analytical skills and learn how to view and interpret the world around us in different ways. Furthermore, we can learn from the past by reading books. In this way, we won't repeat the mistakes of others and can build on their achievements.");
    pre_input.push_back("把这句话翻译成英文：RK3588是新一代高端处理器，具有高算力、低功耗、超强多媒体、丰富数据接口等特点");
    cout << "\n**********************可输入以下问题对应序号获取回答/或自定义输入********************\n"
         << endl;
    for (int i = 0; i < (int)pre_input.size(); i++)
    {
        cout << "[" << i << "] " << pre_input[i] << endl;
    }
    cout << "\n*************************************************************************\n"
         << endl;


    string text;
    //Create the event loop for the main thread, and the WebSocket server
    asio::io_service mainEventLoop;
    WebsocketServer server;

    //Register our network callbacks, ensuring the logic is run on the main thread's event loop
    server.connect([&mainEventLoop, &server](ClientConnection conn)
                   {
                       mainEventLoop.post([conn, &server]()
                                          {
                                              std::clog << "Connection opened." << std::endl;
                                              std::clog << "There are now " << server.numConnections() << " open connections." << std::endl;

                                              //Send a hello message to the client
                                              server.sendMessage(conn, "hello", Json::Value());
                                          });
                   });
    server.disconnect([&mainEventLoop, &server](ClientConnection conn)
                      {
                          mainEventLoop.post([conn, &server]()
                                             {
                                                 std::clog << "Connection closed." << std::endl;
                                                 std::clog << "There are now " << server.numConnections() << " open connections." << std::endl;
                                             });
                      });
    server.message("message", [&mainEventLoop, &server](ClientConnection conn, const Json::Value& args)
    {
        mainEventLoop.post([conn, args, &server]()
                           {
                               std::clog << "message handler on the main thread" << std::endl;
                               std::clog << "Message payload:" << std::endl;
                               for (auto key : args.getMemberNames()) {
                                   std::clog << "\t" << key << ": " << args[key].asString() << std::endl;
                               }

                               // Get the value of 'PROMPT' field
                               if (args.isMember("PROMPT")) {
                                   std::string promptValue = args["PROMPT"].asString();
                                   std::clog << "Value of PROMPT field: " << promptValue << std::endl;
                                   string text = PROMPT_TEXT_PREFIX + promptValue + PROMPT_TEXT_POSTFIX;
                                   printf("user: ");
                                   printf("%s", promptValue.c_str());
                                   printf("%s", text.c_str());
                                   printf("robot: ");

                                   server.sendMessage(conn, "message", );

                                   rkllm_run(llmHandle, text.c_str(), NULL);


                               }



                           });
    });

    //Start the networking thread
    std::thread serverThread([&server]() {
        server.run(PORT_NUMBER);
    });


    //Start the event loop for the main thread
    asio::io_service::work work(mainEventLoop);
    mainEventLoop.run();

    rkllm_destroy(llmHandle);

    return 0;
}
