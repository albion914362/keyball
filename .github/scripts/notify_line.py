#!/usr/bin/env python3
"""通知案内の内容を LINE に送る (LINE Messaging API のプッシュメッセージ)。

使い方:
  環境変数 LINE_CHANNEL_ACCESS_TOKEN, LINE_USER_ID, LINE_MESSAGE を渡して実行する。
  GitHub Actions (notify-line.yml) からも、Claude の作業環境からも同じものを使う。

- 長い文は 4,900 文字ごとに分けて送る (LINE の1通の上限は 5,000 文字)。
- 失敗したときは、原因の見当を日本語で表示して終了コード 1 で終わる。
- トークンは表示しない。
"""
import json
import os
import sys
import urllib.error
import urllib.request
import uuid

API_BASE = os.environ.get("LINE_API_BASE", "https://api.line.me")  # テスト用に差し替え可能
CHUNK = 4900
PER_REQUEST = 5  # 1回の送信でまとめられる吹き出しの数


def fail(msg: str) -> None:
    print(f"::error::{msg}")
    sys.exit(1)


def split_text(text: str) -> list[str]:
    chunks = []
    while text:
        if len(text) <= CHUNK:
            chunks.append(text)
            break
        cut = text.rfind("\n", 0, CHUNK)
        if cut <= 0:
            cut = CHUNK
        chunks.append(text[:cut])
        text = text[cut:].lstrip("\n")
    return chunks


def explain(status: int, body: str) -> str:
    hints = {
        400: "送り先のユーザーID (LINE_USER_ID) がまちがっているか、文面の形がおかしい可能性があります",
        401: "チャネルアクセストークン (LINE_CHANNEL_ACCESS_TOKEN) がまちがっているか、無効になっています",
        403: "このトークンではプッシュメッセージを送る権限がありません",
        429: "今月の無料通数 (200通) を使い切ったか、短時間に送りすぎています",
    }
    hint = hints.get(status, "LINE 側の一時的な障害の可能性があります" if status >= 500 else "原因不明")
    return f"LINE への送信に失敗しました (HTTP {status}): {hint}。LINE の返答: {body[:300]}"


def push(token: str, user_id: str, texts: list[str]) -> None:
    payload = {"to": user_id, "messages": [{"type": "text", "text": t} for t in texts]}
    req = urllib.request.Request(
        f"{API_BASE}/v2/bot/message/push",
        data=json.dumps(payload, ensure_ascii=False).encode("utf-8"),
        headers={
            "Content-Type": "application/json; charset=utf-8",
            "Authorization": f"Bearer {token}",
            "X-Line-Retry-Key": str(uuid.uuid4()),
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=30) as res:
            res.read()
    except urllib.error.HTTPError as e:
        fail(explain(e.code, e.read().decode("utf-8", "replace")))
    except urllib.error.URLError as e:
        fail(f"LINE のサーバーにつながりませんでした: {e.reason}")


def main() -> None:
    token = os.environ.get("LINE_CHANNEL_ACCESS_TOKEN", "").strip()
    user_id = os.environ.get("LINE_USER_ID", "").strip()
    message = os.environ.get("LINE_MESSAGE", "")
    if not token:
        fail("LINE_CHANNEL_ACCESS_TOKEN が登録されていません (GitHub の Secrets を確認してください)")
    if not user_id:
        fail("LINE_USER_ID が登録されていません (GitHub の Secrets を確認してください)")
    if not message.strip():
        fail("送る文面が空です")
    chunks = split_text(message.replace("\r\n", "\n"))
    for i in range(0, len(chunks), PER_REQUEST):
        push(token, user_id, chunks[i:i + PER_REQUEST])
    print(f"LINE に送りました (吹き出し {len(chunks)} 個、{len(message)} 文字)")


if __name__ == "__main__":
    main()
