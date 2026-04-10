#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试 Codex API 配置
验证自定义 API 端点是否正常工作
"""

import requests
import json
import os

# 禁用代理（如果系统配置了代理但不可用）
os.environ['NO_PROXY'] = '*'
os.environ['no_proxy'] = '*'

# 从配置文件读取的设置
BASE_URL = "https://realmrouter.cn/v1"
API_KEY = "sk-Fh7wdut5PYCsUA00nXuOYb5exOjwrmPkccay7OQxkjh9zNbO"
MODEL = "gpt-5.3-codex"

def test_api_connection():
    """测试 API 连接"""
    print("=" * 60)
    print("测试 Codex API 配置")
    print("=" * 60)
    print(f"\n配置信息:")
    print(f"  Base URL: {BASE_URL}")
    print(f"  Model: {MODEL}")
    print(f"  API Key: {API_KEY[:20]}...{API_KEY[-10:]}")
    print("\n" + "-" * 60)
    
    # 构建完整的 API 端点
    endpoint = f"{BASE_URL}/chat/completions"
    
    # 准备请求头
    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {API_KEY}"
    }
    
    # 准备测试请求数据
    data = {
        "model": MODEL,
        "messages": [
            {
                "role": "user",
                "content": "请用中文回复：你好，这是一个API连接测试。"
            }
        ],
        "max_tokens": 100,
        "temperature": 0.7
    }
    
    print(f"\n正在测试连接到: {endpoint}")
    print("发送测试请求...")
    
    try:
        # 发送请求
        response = requests.post(
            endpoint,
            headers=headers,
            json=data,
            timeout=30
        )
        
        print(f"\n响应状态码: {response.status_code}")
        
        if response.status_code == 200:
            print("✓ API 连接成功！")
            result = response.json()
            
            # 显示响应内容
            if "choices" in result and len(result["choices"]) > 0:
                message = result["choices"][0].get("message", {})
                content = message.get("content", "")
                print(f"\nAPI 响应内容:")
                print(f"  {content}")
                
            # 显示使用情况
            if "usage" in result:
                usage = result["usage"]
                print(f"\n令牌使用情况:")
                print(f"  提示令牌: {usage.get('prompt_tokens', 'N/A')}")
                print(f"  完成令牌: {usage.get('completion_tokens', 'N/A')}")
                print(f"  总令牌: {usage.get('total_tokens', 'N/A')}")
            
            print("\n" + "=" * 60)
            print("✓ 测试通过！你的 API 配置正常工作。")
            print("=" * 60)
            return True
            
        else:
            print(f"✗ API 请求失败")
            print(f"\n错误响应:")
            try:
                error_data = response.json()
                print(json.dumps(error_data, indent=2, ensure_ascii=False))
            except:
                print(response.text)
            
            print("\n" + "=" * 60)
            print("✗ 测试失败！请检查配置。")
            print("=" * 60)
            return False
            
    except requests.exceptions.Timeout:
        print("\n✗ 请求超时")
        print("可能的原因：")
        print("  1. 网络连接问题")
        print("  2. API 服务器响应慢")
        print("  3. 防火墙阻止连接")
        return False
        
    except requests.exceptions.ConnectionError as e:
        print(f"\n✗ 连接错误: {e}")
        print("可能的原因：")
        print("  1. Base URL 不正确")
        print("  2. 网络连接问题")
        print("  3. API 服务器不可用")
        return False
        
    except Exception as e:
        print(f"\n✗ 发生错误: {type(e).__name__}: {e}")
        return False

def test_models_endpoint():
    """测试模型列表端点（可选）"""
    print("\n" + "-" * 60)
    print("测试模型列表端点...")
    
    endpoint = f"{BASE_URL}/models"
    headers = {
        "Authorization": f"Bearer {API_KEY}"
    }
    
    try:
        response = requests.get(endpoint, headers=headers, timeout=10)
        if response.status_code == 200:
            print("✓ 模型列表端点可访问")
            models = response.json()
            if "data" in models:
                print(f"  可用模型数量: {len(models['data'])}")
                # 显示前几个模型
                for i, model in enumerate(models['data'][:5]):
                    model_id = model.get('id', 'unknown')
                    print(f"    - {model_id}")
                if len(models['data']) > 5:
                    print(f"    ... 还有 {len(models['data']) - 5} 个模型")
        else:
            print(f"  模型列表端点返回状态码: {response.status_code}")
    except Exception as e:
        print(f"  无法访问模型列表端点: {e}")

if __name__ == "__main__":
    # 运行主测试
    success = test_api_connection()
    
    # 尝试测试模型列表（可选）
    try:
        test_models_endpoint()
    except:
        pass
    
    # 返回退出码
    exit(0 if success else 1)
