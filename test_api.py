#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
API测试脚本
测试 realmrouter.cn API 是否可以正常请求
"""

import requests
import json
import os

# API配置
BASE_URL = "https://realmrouter.cn/v1"
API_KEY = "sk-Fh7wdut5PYCsUA00nXuOYb5exOjwrmPkccay7OQxkjh9zNbO"
MODEL = "gpt-5.4"

def test_api():
    """测试API连接"""
    print("=" * 60)
    print("开始测试 API 连接...")
    print(f"Base URL: {BASE_URL}")
    print(f"Model: {MODEL}")
    print("=" * 60)
    
    # 检查并清除代理设置
    print("\n检查代理设置...")
    http_proxy = os.environ.get('HTTP_PROXY') or os.environ.get('http_proxy')
    https_proxy = os.environ.get('HTTPS_PROXY') or os.environ.get('https_proxy')
    
    if http_proxy or https_proxy:
        print(f"检测到代理设置: HTTP={http_proxy}, HTTPS={https_proxy}")
        print("尝试禁用代理进行直连...")
    else:
        print("未检测到系统代理设置")
    
    # 设置请求头
    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json"
    }
    
    # 构建请求数据
    data = {
        "model": MODEL,
        "messages": [
            {
                "role": "user",
                "content": "你好，这是一个测试请求。请简单回复确认收到。"
            }
        ],
        "max_tokens": 100
    }
    
    # 禁用代理
    session = requests.Session()
    session.trust_env = False  # 忽略环境变量中的代理设置
    
    try:
        print("\n发送测试请求...")
        # 发送POST请求到chat completions端点
        response = session.post(
            f"{BASE_URL}/chat/completions",
            headers=headers,
            json=data,
            timeout=30,
            proxies={"http": None, "https": None}  # 明确禁用代理
        )
        
        print(f"\n响应状态码: {response.status_code}")
        
        if response.status_code == 200:
            print("✓ API 请求成功！")
            result = response.json()
            print("\n响应内容:")
            print(json.dumps(result, indent=2, ensure_ascii=False))
            
            # 提取并显示回复内容
            if "choices" in result and len(result["choices"]) > 0:
                message = result["choices"][0].get("message", {})
                content = message.get("content", "")
                print("\n" + "=" * 60)
                print("AI 回复:")
                print(content)
                print("=" * 60)
            
            return True
        else:
            print(f"✗ API 请求失败")
            print(f"错误信息: {response.text}")
            return False
            
    except requests.exceptions.Timeout:
        print("✗ 请求超时 (30秒)")
        print("可能原因: 网络连接慢或服务器响应慢")
        return False
    except requests.exceptions.ConnectionError as e:
        print(f"✗ 连接错误: {e}")
        print("\n可能的解决方案:")
        print("1. 检查网络连接是否正常")
        print("2. 确认 realmrouter.cn 域名可以访问")
        print("3. 检查防火墙设置")
        print("4. 如果在公司网络，可能需要配置代理")
        return False
    except requests.exceptions.SSLError as e:
        print(f"✗ SSL证书错误: {e}")
        print("可能原因: SSL证书验证失败")
        return False
    except Exception as e:
        print(f"✗ 发生错误: {type(e).__name__}: {e}")
        return False

def test_connectivity():
    """测试基本网络连通性"""
    print("\n" + "=" * 60)
    print("测试网络连通性...")
    print("=" * 60)
    
    session = requests.Session()
    session.trust_env = False
    
    try:
        # 测试DNS解析和基本连接
        response = session.get(
            "https://realmrouter.cn",
            timeout=10,
            proxies={"http": None, "https": None}
        )
        print(f"✓ 可以连接到 realmrouter.cn (状态码: {response.status_code})")
        return True
    except Exception as e:
        print(f"✗ 无法连接到 realmrouter.cn: {e}")
        return False

if __name__ == "__main__":
    # 先测试基本连通性
    connectivity_ok = test_connectivity()
    
    if connectivity_ok:
        print("\n继续测试 API...")
        success = test_api()
    else:
        print("\n跳过 API 测试（基本连接失败）")
        success = False
    
    print("\n" + "=" * 60)
    if success:
        print("测试结果: 成功 ✓")
        print("API 可以正常使用")
    else:
        print("测试结果: 失败 ✗")
        print("API 无法连接，请检查网络设置")
    print("=" * 60)
