#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DOCKER_DIR="$PROJECT_DIR/docker"

echo "============================================"
echo "  C++ 直播项目 - Docker 开发环境"
echo "============================================"
echo ""

case "${1:-help}" in
    build)
        echo "[1/4] 构建 Docker 镜像..."
        cd "$DOCKER_DIR"
        docker compose build chatroom-server
        echo ""
        echo "✅ 镜像构建完成!"
        ;;

    up)
        echo "[2/4] 启动所有服务..."
        cd "$DOCKER_DIR"
        docker compose up -d mysql redis
        echo "等待 MySQL 就绪..."
        sleep 10
        docker compose up -d chatroom-server
        echo ""
        docker compose ps
        echo ""
        echo "✅ 服务已启动!"
        echo ""
        echo "  MySQL:   localhost:3306 (root/root123)"
        echo "  Redis:   localhost:6379"
        echo "  Server:  localhost:8900"
        echo ""
        echo "进入容器: docker exec -it chatroom-dev bash"
        ;;

    down)
        echo "[停止所有服务...]"
        cd "$DOCKER_DIR"
        docker compose down
        echo "✅ 已停止"
        ;;

    shell)
        cd "$DOCKER_DIR"
        docker compose exec chatroom-server bash
        ;;

    build-project)
        echo "[在容器内编译项目...]"
        cd "$DOCKER_DIR"
        docker compose exec chatroom-server bash -c "
            cd /workspace/chatroom-server && \
            mkdir -p build && \
            cd build && \
            cmake .. -DCMAKE_BUILD_TYPE=Release && \
            cmake --build . --parallel \$(nproc) && \
            echo '' && \
            echo '✅ 编译完成! 可执行文件: build/chatroom_server'
        "
        ;;

    run)
        echo "[运行服务器...]"
        cd "$DOCKER_DIR"
        docker compose exec chatroom-server bash -c "
            cd /workspace/chatroom-server && \
            ./build/chatroom_server
        "
        ;;

    logs)
        cd "$DOCKER_DIR"
        docker compose logs -f chatroom-server
        ;;

    clean)
        echo "[清理构建产物和容器...]"
        cd "$DOCKER_DIR"
        docker compose down -v --rmi local
        rm -rf "$PROJECT_DIR/chatroom-server/build"
        echo "✅ 清理完成"
        ;;

    status)
        cd "$DOCKER_DIR"
        docker compose ps
        echo ""
        docker compose exec chatroom-server bash -c "
            gcc --version | head -1
            cmake --version | head -1
            echo ''
            echo '依赖库:'
            dpkg -l | grep -E 'libspdlog|nlohmann|libfmt|libprotobuf|libhiredis|libmysql|gtest' | awk '{print \"  ✅ \"\$2}' || true
        "
        ;;

    *)
        echo "用法: $0 {命令}"
        echo ""
        echo "命令:"
        echo "  build         构建开发环境镜像 (首次使用)"
        echo "  up            启动全部服务 (MySQL + Redis + 开发容器)"
        echo "  down          停止所有服务"
        echo "  shell         进入开发容器终端"
        echo "  build-project 在容器内编译项目"
        echo "  run           运行服务器"
        echo "  logs          查看日志"
        echo "  status        查看状态和环境信息"
        echo "  clean         清理所有数据(慎用!)"
        echo ""
        echo "快速开始:"
        echo "  $0 build       # 首次: 构建镜像 (~5-10分钟)"
        echo "  $0 up          # 启动服务"
        echo "  $0 shell       # 进入容器写代码"
        echo ""
        echo "VSCode/Trae 连接方式:"
        echo "  打开 chatroom-server 目录 → F1 → Dev Containers: Reopen in Container"
        ;;
esac
