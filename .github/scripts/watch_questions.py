# 開発者への質問に回答が付いたかを確認し、新しい書き込みがあれば Issue
# (ラベル「質問の回答」) で知らせる。書き込みが編集されたときは、前回確認
# した文との差分を Issue にコメントする。
# 見張る質問 = QUESTION_AUTHOR が他人のリポジトリに立てた Discussion・Issue
#            + .github/watch-questions.txt に書いたもの
import base64, difflib, json, os, subprocess, sys

ME = os.environ["QUESTION_AUTHOR"]
LIST_FILE = ".github/watch-questions.txt"
SUMMARY = os.environ.get("GITHUB_STEP_SUMMARY", "/dev/null")


def log(msg):
    print(msg)
    with open(SUMMARY, "a") as f:
        f.write(msg + "\n\n")


def gh(*args):
    return subprocess.run(["gh", *args], check=True, capture_output=True, text=True).stdout


def graphql(query, **variables):
    args = ["api", "graphql", "-f", f"query={query}"]
    for k, v in variables.items():
        args += ["-F", f"{k}={v}"]
    return json.loads(gh(*args))["data"]


def login(node):
    return (node.get("author") or {}).get("login", "ghost")


def find_targets():
    targets = set()
    with open(LIST_FILE) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#"):
                kind, repo, number = line.split()
                targets.add((kind, repo, int(number)))
    for kind, search_type, extra in (("discussion", "DISCUSSION", ""), ("issue", "ISSUE", " is:issue")):
        data = graphql(
            """query($q: String!) { search(type: %s, query: $q, first: 100) { nodes {
                 ... on Discussion { number repository { nameWithOwner owner { login } } }
                 ... on Issue { number repository { nameWithOwner owner { login } } } } } }""" % search_type,
            q=f"author:{ME}{extra}")
        found = [(kind, n["repository"]["nameWithOwner"], n["number"]) for n in data["search"]["nodes"]
                 if n and n["repository"]["owner"]["login"] != ME]
        log(f"自動で見つけた {kind}: {len(found)} 件")
        targets.update(found)
    return sorted(targets)


def fetch(kind, repo, number):
    owner, name = repo.split("/")
    post = "author { login } createdAt url body"
    if kind == "discussion":
        d = graphql("""query($owner: String!, $name: String!, $number: Int!) {
            repository(owner: $owner, name: $name) { discussion(number: $number) {
              title url author { login }
              comments(last: 50) { nodes { %s replies(last: 50) { nodes { %s } } } } } } }""" % (post, post),
            owner=owner, name=name, number=number)["repository"]["discussion"]
        posts = [p for c in d["comments"]["nodes"] for p in [c, *c["replies"]["nodes"]]]
    else:
        d = graphql("""query($owner: String!, $name: String!, $number: Int!) {
            repository(owner: $owner, name: $name) { issue(number: $number) {
              title url author { login } comments(last: 100) { nodes { %s } } } } }""" % post,
            owner=owner, name=name, number=number)["repository"]["issue"]
        posts = d["comments"]["nodes"]
    asker = login(d)
    # 質問者以外の書き込み (コメントと返信) を新しい順に並べる
    posts = sorted((p for p in posts if login(p) != asker), key=lambda p: p["createdAt"], reverse=True)
    return d["title"], d["url"], posts


def snapshot_of(body):
    start = body.find("<!-- snapshot: ")
    if start < 0:
        return None
    return json.loads(base64.b64decode(body[start + 15:body.index(" -->", start)]))


def diff_md(old, new):
    old = {p["url"]: p for p in old}
    out = []
    for p in new:
        before = old.get(p["url"])
        if not before or before["body"] == p["body"]:
            continue
        diff = difflib.unified_diff(before["body"].splitlines(), p["body"].splitlines(),
                                    "修正前", "修正後", lineterm="")
        out.append(f"### {p['author']} の書き込みが修正されました\n\n[元の書き込み]({p['url']})\n\n"
                   "```diff\n" + "\n".join(diff) + "\n```\n")
    return "\n".join(out)


def check(kind, repo, number, issues):
    title, url, posts = fetch(kind, repo, number)
    log(f"{kind} {repo}#{number}「{title}」: 質問者以外の書き込み {len(posts)} 件")
    if not posts:
        return
    # Issue #16 など既存の Issue と同じ印を使う (discussion は従来どおり)
    marker = f"{'discussion' if kind == 'discussion' else 'issue'}-watch: {repo}#{number} "
    latest = posts[0]["url"]
    snapshot = [{"url": p["url"], "author": login(p), "body": p["body"]} for p in posts]
    body = (f"<!-- {marker}latest: {latest} -->\n"
            f"質問「[{title}]({url})」に回答・返信が付きました（質問者以外の書き込み {len(posts)} 件）。\n\n"
            + "".join(f"### {login(p)}（{p['createdAt'][:10]}）\n\n{p['body']}\n\n[元の書き込み]({p['url']})\n\n"
                      for p in posts[:5])
            + f"<!-- snapshot: {base64.b64encode(json.dumps(snapshot).encode()).decode()} -->\n")
    with open("body.md", "w") as f:
        f.write(body)

    issue = next((i for i in issues if marker in i["body"]), None)
    if issue is None:
        gh("issue", "create", "--title", f"質問に回答あり: {title}", "--label", "質問の回答", "--body-file", "body.md")
        log("→ 新しい回答を検知 (Issue を作成)")
        return
    num = str(issue["number"])
    if f"latest: {latest} " not in issue["body"]:
        subprocess.run(["gh", "issue", "reopen", num])
        gh("issue", "edit", num, "--body-file", "body.md", "--remove-label", "解説済み")
        gh("issue", "comment", num, "--body", "新しい書き込みがありました。本文を最新の内容に書き換えました。")
        log(f"→ 新しい書き込みを検知 (Issue #{num})")
        return
    # 新しい書き込みがないときは、前回の本文と比べて編集を探す
    old = snapshot_of(issue["body"])
    if old is None:
        # 本文の記録がまだない Issue は、記録だけ足して終わる (通知しない)
        gh("issue", "edit", num, "--body-file", "body.md")
        log("→ 新しい書き込みはない (編集検知用の記録を追加)")
        return
    diff = diff_md(old, snapshot)
    if not diff:
        log("→ 新しい書き込みも修正もない")
        return
    with open("diff.md", "w") as f:
        f.write(diff)
    subprocess.run(["gh", "issue", "reopen", num])
    gh("issue", "edit", num, "--body-file", "body.md", "--remove-label", "解説済み")
    gh("issue", "comment", num, "--body-file", "diff.md")
    log(f"→ 書き込みの修正を検知 (Issue #{num})")


def main():
    gh("label", "create", "質問の回答", "--color", "1D76DB", "--description", "開発者への質問に回答があった", "--force")
    gh("label", "create", "解説済み", "--color", "0E8A16", "--description", "Claude が解説を書いた", "--force")
    targets = find_targets()
    log(f"見張っている質問: {len(targets)} 件")
    issues = json.loads(gh("issue", "list", "--label", "質問の回答", "--state", "all", "--limit", "200",
                           "--json", "number,body"))
    failed = 0
    for kind, repo, number in targets:
        try:
            check(kind, repo, number, issues)
        except subprocess.CalledProcessError as e:
            failed += 1
            log(f"{kind} {repo}#{number} の確認に失敗: {e.stderr.strip()}")
    # 1 件でも失敗したら run を失敗にして、Claude の定期タスクに気づかせる
    sys.exit(1 if failed else 0)


main()
