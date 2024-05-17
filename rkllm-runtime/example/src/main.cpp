// Copyright (c) 2024 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
        std::function<void(const char*)>* sendMessageWrapperPtr = reinterpret_cast<std::function<void(const char*)>*>(userdata);

        // 调用 sendMessageWrapper 函数对象，并传递 text 参数
        if (sendMessageWrapperPtr) {
            (*sendMessageWrapperPtr)(text);
        }
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

//    string text;
//    while (true)
//    {
//        std::string input_str;
//        printf("\n");
//        printf("user: ");
//        std::getline(std::cin, input_str);
//        if (input_str == "exit")
//        {
//            break;
//        }
//        for (int i = 0; i < (int)pre_input.size(); i++)
//        {
//            if (input_str == to_string(i))
//            {
//                input_str = pre_input[i];
//                cout << input_str << endl;
//            }
//        }
//        string text = PROMPT_TEXT_PREFIX + input_str + PROMPT_TEXT_POSTFIX;
//        printf("robot: ");
//        rkllm_run(llmHandle, text.c_str(), NULL);
//    }

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
//            for (auto key : args.getMemberNames()) {
//                std::clog << "\t" << key << ": " << args[key].asString() << std::endl;
//            }

            // Get the value of 'PROMPT' field
            if (args.isMember("PROMPT")) {
                std::string promptValue = args["PROMPT"].asString();
//                std::clog << "Value of PROMPT field: " << promptValue << std::endl;
                string text = PROMPT_TEXT_PREFIX + promptValue + PROMPT_TEXT_POSTFIX;
//                printf("user: ");
//                printf("%s", promptValue.c_str());
//                printf("%s", text.c_str());
//                printf("robot: ");

                // 定义 lambda 函数，捕获 server 和 conn
                auto sendMessageFunc = [&server, &conn](const char* text) {
                    if (text && text[0] != '\0') {
                        Json::Value message;
                        message["text"] = text;

                        // 添加打印语句
                        std::cout << "Sending message: " << message.toStyledString() << std::endl;

                        try {
                            server.sendMessage(conn, "message", message);
                        } catch (const websocketpp::exception& e) {
                            // 捕获并打印异常信息
                            std::cerr << "Error sending message: " << e.what() << std::endl;
                            throw;
                        }
                    }
                };

                // 转换成 std::function
                std::function<void(const char*)> sendMessageWrapper = sendMessageFunc;

                // 调用 rkllm_run，将 sendMessageWrapper 作为 userdata 参数传递
                rkllm_run(llmHandle, text.c_str(), reinterpret_cast<void*>(&sendMessageWrapper));
            }


//        printf("robot: ");
            //Echo the message pack to the client

        });
    });

    //Start the networking thread
    std::thread serverThread([&server]() {
        server.run(PORT_NUMBER);
    });

    //Start a keyboard input thread that reads from stdin
//    std::thread inputThread([&server, &mainEventLoop]()
//    {
//        string input;
//        while (1)
//        {
//            //Read user input from stdin
//            std::getline(std::cin, input);
//
//            //Broadcast the input to all connected clients (is sent on the network thread)
//            Json::Value payload;
//            payload["input"] = input;
//            server.broadcastMessage("userInput", payload);
//
//            //Debug output on the main thread
//            mainEventLoop.post([]() {
//                std::clog << "User input debug output on the main thread" << std::endl;
//            });
//        }
//    });

    //Start the event loop for the main thread
    asio::io_service::work work(mainEventLoop);
    mainEventLoop.run();

    rkllm_destroy(llmHandle);

    return 0;
}
