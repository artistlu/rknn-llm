# 使用基础的 Ubuntu 镜像
FROM debian:11

# 将可执行文件复制到容器中的 /app 目录
COPY llm_demo /app/llm_demo
COPY librkllmrt.so /lib/aarch64-linux-gnu/librkllmrt.so
COPY Qwen-1_8B-Chat.rkllm /app/Qwen-1_8B-Chat.rkllm

# 设置工作目录为 /app
WORKDIR /app

CMD ["/app/llm_demo", "/app/Qwen-1_8B-Chat.rkllm"]