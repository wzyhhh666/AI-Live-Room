
# 阶段一（2-3 周）：自建 SMTP + POP3 私有邮箱服务（可与 Foxmail 通信）

> 生成日期：2026-05-29

## 0. 你的阶段一目标（最小可跑通）

### 0.1 目标
实现一个**局域网/同域内**可用的基础邮箱服务：
- 标准邮箱客户端（Foxmail 等）可以：
  - 通过 **SMTP** 向你的服务端投递邮件
  - 通过 **POP3** 从你的服务端拉取邮件
- 你的自研客户端（Windows Qt 5.12.11）也能用同样的 SMTP/POP3 与服务端交互
- 支持 **纯文本邮件**（RFC 5322 最小子集），先不做 HTML/附件

### 0.2 非目标（阶段一明确不做）
这套“私有 SMTP+POP3 邮箱服务”阶段一 **不能**和 QQ/163 等公网邮箱“互相收发”，原因是你没有做互联网邮件互通必需的 DNS/MX、出站投递信誉、反垃圾、队列、DKIM/SPF/DMARC 等。

> 若你后续必须对接 QQ/163，推荐走“网关模式”：你写一个 Mail Gateway 作为 IMAP/SMTP 客户端去登录 QQ/163，再给你的 Qt 客户端提供 API。那是阶段二/阶段三的内容。

---

## 1. 环境与技术约束（按你指定的来）

### 1.1 服务端
- OS：Linux 虚拟机 Ubuntu 22.04
- Qt：Qt 5.9.9（注意：仅作为工程/依赖环境，不强依赖 Qt 网络栈）
- 网络层：复用你网盘项目的 **epoll LT + 非阻塞 IO + 线程池**
- 协议：SMTP Server + POP3 Server（纯文本，先不加 TLS）
- 存储：
  - MySQL：用户信息、邮件元数据
  - 本地文件：邮件内容 `.eml`

### 1.2 客户端
- OS：Windows
- Qt：Qt 5.12.11
- 功能：
  - SMTP 发信（投递到你的服务端）
  - POP3 收信（从你的服务端拉取）
  - 草稿/发件队列（阶段一最小化：**客户端本地 SQLite** 即可）

> 说明：草稿/发件箱在标准协议里一般依赖 IMAP 文件夹语义；你阶段一做 POP3，最省事的是先在客户端本地做草稿/Outbox。等阶段二上 IMAP 再做“服务端草稿箱”。

---

## 2. 总体架构（最小可维护版）

### 2.1 进程与模块
建议服务端先做成一个进程 `simplemaild`，同时监听两类端口：
- SMTP Listener（建议开发期用 `2525`，避免 root 绑定 25）
- POP3 Listener（建议开发期用 `1110`，避免 root 绑定 110）

模块划分（只列阶段一必需）：
- `net/`：EpollReactor、Connection、Buffer、Timer、ThreadPool
- `smtp/`：SmtpSession（状态机）、CommandParser、MessageCollector
- `pop3/`：Pop3Session（状态机）、CommandParser
- `store/`：MySqlRepo（用户/元信息）、MailFileStore（.eml 落盘/读回）
- `auth/`：UserAuth（用户名/密码验证）
- `common/`：日志、配置、工具（CRLF、时间、ID 生成）

### 2.2 数据流
#### SMTP（投递）
1) 客户端连 SMTP -> AUTH（最小版用 `AUTH PLAIN`）
2) `MAIL FROM` / `RCPT TO` / `DATA`
3) 服务端把 DATA 收到的 **原始 RFC 5322 文本**落盘为 `.eml`
4) 写 MySQL 元信息（收件人、发件人、主题、时间、文件路径、大小、状态）

#### POP3（取信）
1) 客户端连 POP3 -> `USER`/`PASS`
2) `STAT`/`LIST` 拉列表
3) `RETR` 拉取某封邮件的 `.eml` 原文
4) `DELE` 标记删除（POP3 语义：直到 `QUIT` 才真正删除）

---

## 3. 协议层：阶段一“最小子集”

> 你说“先实现初步框架，后续再补规范细节”。下面就是能跑通 Foxmail 的最小集。

### 3.1 SMTP Server（最小命令集）
必须支持：
- `HELO`（可选支持 `EHLO`，建议支持；但先不实现复杂扩展也可以）
- `AUTH PLAIN`（建议作为最小起点）
- `MAIL FROM:<...>`
- `RCPT TO:<...>`（支持多个收件人，至少 1 个）
- `DATA`（接收正文，直到 `<CRLF>.<CRLF>`）
- `RSET`、`NOOP`、`QUIT`

必须实现的细节：
- 协议行以 `\r\n` 结尾
- `DATA` 阶段的 dot-stuffing：收到以 `.` 开头的行需要去掉前置 `.`
- 只支持“同域收件人”：例如 `@local.test`，收件人不在域内直接 `550 No such user`

建议的最小响应码：
- 连接欢迎：`220`
- 命令成功：`250`
- 认证成功：`235`
- 需要认证：`530`
- 参数错误：`501`
- 用户不存在：`550`
- 进入 DATA：`354 End data with <CRLF>.<CRLF>`

### 3.2 POP3 Server（最小命令集）
必须支持：
- `USER` / `PASS`
- `STAT`（返回邮件数量与总字节数）
- `LIST`（列表：编号与大小）
- `RETR <n>`（返回第 n 封邮件原文）
- `DELE <n>`（标记删除）
- `NOOP`、`QUIT`

建议尽早支持（但不强制）：
- `UIDL`（给客户端稳定 ID；不做也能跑，但很多客户端会更“舒服”）

POP3 必须实现的细节：
- 服务器行结束 `\r\n`
- 多行返回以 `\r\n.\r\n` 结束
- `DELE` 只标记，`QUIT` 才真正删除（或移动到垃圾箱）

---

## 4. 纯文本邮件格式：存什么、解析什么（阶段一最小）

### 4.1 阶段一“存储策略”
- SMTP `DATA` 收到什么，就把什么原样存成 `.eml`（少做聪明事，先保证互通）
- MySQL 只抽取最少字段用于列表：
  - `from`、`to`、`subject`、`date`、`size`、`path`

### 4.2 阶段一“解析策略”（可非常保守）
- 先只解析 Header 区：读取到空行结束
- 支持 header folding（以空格/Tab 开头的续行拼接）
- `Subject` 先不做 RFC2047 也能跑通英文；若你想一步到位，阶段一就加 RFC2047 解码

---

## 5. 存储层：MySQL + 本地文件（最小可用模型）

### 5.1 文件存储布局（建议）
以“每个用户一个目录”最简单：
- 根目录：`/var/lib/simplemail/mail/`
- 用户目录：`/var/lib/simplemail/mail/{user_id}/`
- 邮件文件：`{msg_id}.eml`

写文件要点：
- 先写临时文件（`.tmp`），写完 `fsync` 后再原子 `rename`，避免崩溃产生半包文件

### 5.2 MySQL 表（最小可跑通）

#### `users`
- `id` BIGINT PK
- `username` VARCHAR UNIQUE（建议就是邮箱前缀，如 `alice`）
- `email` VARCHAR UNIQUE（如 `alice@local.test`）
- `password_salt` VARBINARY
- `password_hash` VARBINARY（阶段一可以先用 `SHA-256(salt + password)`，后续再换更强 KDF）
- `created_at` DATETIME

#### `messages`
- `id` BIGINT PK
- `owner_user_id` BIGINT（收件箱归属用户）
- `mail_from` VARCHAR
- `rcpt_to` VARCHAR（阶段一可先存一个；若多收件人，建议单独建 `message_rcpt` 表）
- `subject` VARCHAR
- `date_raw` VARCHAR（先原样存 Header 的 Date）
- `size_bytes` BIGINT
- `eml_path` VARCHAR
- `deleted` TINYINT（0/1，POP3 的删除可落这里）
- `created_at` DATETIME

> 阶段一最小化：每个收件人各写一条 `messages` 记录，并各自落一份文件（省去共享与引用计数复杂度）。后续再优化。

---

## 6. 用户认证（阶段一最小）

### 6.1 SMTP 认证
- 最小建议：支持 `AUTH PLAIN`
- 服务器端验证 `username/password` 后把会话标记为已认证

### 6.2 POP3 认证
- `USER` + `PASS`，验证通过进入 TRANSACTION 状态

### 6.3 安全边界（阶段一必须写在 README 里）
- 阶段一默认**不加 TLS**：只允许内网测试，不要暴露公网
- 后续上 TLS：SMTP 用 STARTTLS/SMTPS，POP3 用 STLS/POP3S

---

## 7. 客户端（Windows Qt 5.12.11）阶段一最小功能

### 7.1 必做页面/功能
- 登录（输入服务器地址、SMTP/POP3 端口、用户名密码）
- 收件箱列表（POP3：STAT/LIST/RETR，解析 Subject/From/Date 仅用于展示）
- 邮件详情（显示 `text/plain`）
- 写信（To/Subject/Body）-> SMTP 投递

### 7.2 草稿/发件箱（阶段一最小做法）
- 草稿：客户端本地 SQLite 保存 `to/subject/body/updated_at`
- Outbox：本地队列（失败可重试），发送成功后写入 Sent（仍是本地）

---

## 8. 2-3 周实现顺序（强执行版）

### 第 1 周：SMTP 投递闭环
1) 起服务端骨架：epoll accept + connection + readLine(CRLF)
2) 实现 SMTP 会话状态机（不做 TLS，只做 AUTH PLAIN + MAIL/RCPT/DATA）
3) `.eml` 落盘 + MySQL 写入 `users/messages`
4) 用 `telnet`/`nc` 手工投递一封邮件验证落库与落盘

验收：
- 手工 SMTP 能投递成功，收件用户目录下出现 `.eml`，MySQL 有 messages

### 第 2 周：POP3 拉取闭环 + Foxmail 对接
1) 实现 POP3：USER/PASS/STAT/LIST/RETR/DELE/QUIT
2) RETR 时直接读 `.eml` 原文返回
3) DELE 标记删除，QUIT 落地删除（或只更新 deleted）
4) Foxmail 配置 POP3/SMTP 指向你的服务端（开发端口 1110/2525）验证收发

验收：
- Foxmail 能登录、看到列表、打开内容、删除邮件

### 第 3 周：你的 Qt 客户端最小 UI + 稳定性
1) Qt 客户端做登录/列表/详情/写信
2) 增加本地草稿/Outbox（SQLite）
3) 补齐服务器端日志、超时、错误码一致性

验收：
- 你的 Qt 客户端能发信到服务端，再用 POP3 收到
- 崩溃/重启后邮件仍在（文件 + MySQL）

---

## 9. 阶段一框架能做到什么/不能做到什么（结论）

### 能做到
- 你的客户端 ↔ 你的服务端：通过 SMTP/POP3 正常收发
- Foxmail 等标准客户端 ↔ 你的服务端：只要按协议子集实现正确，就能互通

### 做不到（除非你进入下一阶段）
- 与 QQ/163 等公网邮箱来回收发：阶段一不具备互联网邮件互通条件

---

## 10. 下一步（如果你确认阶段一，就从这里开敲）

你只需要先把两件事做出来：
1) SMTP：`AUTH PLAIN + MAIL/RCPT/DATA` 并落盘 `.eml`
2) POP3：`USER/PASS + LIST/RETR` 能把 `.eml` 原样吐给客户端

做到这两点，你的服务端框架就成立了，后面再逐步补规范细节（EHLO 扩展、UIDL、编码、TLS、附件、IMAP 等）。

---

## 11. 附录：MySQL 建表 SQL（最小可跑通）

> 目标：先让 SMTP 投递能落库，POP3 能按用户列出并 RETR 邮件。

```sql
-- 建议统一使用 utf8mb4，避免后续中文主题/昵称出问题
CREATE DATABASE IF NOT EXISTS simplemail
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_0900_ai_ci;

USE simplemail;

CREATE TABLE IF NOT EXISTS users (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  username VARCHAR(64) NOT NULL,
  email VARCHAR(255) NOT NULL,
  password_salt VARBINARY(32) NOT NULL,
  password_hash VARBINARY(32) NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_users_username (username),
  UNIQUE KEY uk_users_email (email)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS messages (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  owner_user_id BIGINT UNSIGNED NOT NULL,
  mail_from VARCHAR(255) NOT NULL,
  rcpt_to VARCHAR(255) NOT NULL,
  subject VARCHAR(512) NOT NULL DEFAULT '',
  date_raw VARCHAR(128) NOT NULL DEFAULT '',
  size_bytes BIGINT UNSIGNED NOT NULL,
  eml_path VARCHAR(1024) NOT NULL,
  pop3_deleted TINYINT(1) NOT NULL DEFAULT 0,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_messages_owner_created (owner_user_id, created_at),
  KEY idx_messages_owner_deleted (owner_user_id, pop3_deleted),
  CONSTRAINT fk_messages_owner FOREIGN KEY (owner_user_id) REFERENCES users(id)
    ON DELETE CASCADE
) ENGINE=InnoDB;
```

### 11.1 初始化一个测试用户（示例）
你服务端最终会提供“创建用户”的管理接口或命令行工具；阶段一最小化可以先手动插入。

你实现密码哈希时建议固定为：
- `password_hash = SHA256(password_salt + password)`

示意 SQL（salt/hash 请用你实际生成的二进制值）：

```sql
-- 下面只是占位示例；实际请在你的工具里生成 salt/hash
INSERT INTO users(username, email, password_salt, password_hash)
VALUES ('alice', 'alice@local.test', UNHEX('00112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDDEEFF'), UNHEX('FFEEDDCCBBAA99887766554433221100FFEEDDCCBBAA99887766554433221100'));
```

---

## 12. 附录：SMTP / POP3 会话状态机（伪代码）

> 目标：让你能按“状态机 + 行协议”直接编码，不被细节拖住。

### 12.1 SMTP Session（最小状态机）

状态：
- `kGreet` -> `kAuth` -> `kMailTx` -> `kData` -> `kMailTx` ... -> `kQuit`

伪代码：

```text
onConnect:
  send("220 simplemaild ESMTP\r\n")
  state = kAuth

onLine(line):
  cmd, args = parseSmtpCommand(line)

  if cmd in {"NOOP"}: send("250 OK\r\n"); return
  if cmd in {"RSET"}: resetTransaction(); send("250 OK\r\n"); state = kMailTx; return
  if cmd in {"QUIT"}: send("221 Bye\r\n"); close(); return

  if state == kAuth:
    if cmd in {"HELO","EHLO"}: send("250-Hello\r\n250 AUTH PLAIN\r\n"); return
    if cmd == "AUTH" and args startsWith "PLAIN":
       b64 = extractB64(args)
       decoded = base64Decode(b64)  # format: \0username\0password
       username, password = splitByNul(decoded)
       if authOk(username, password):
          authedUser = username
          send("235 Authentication successful\r\n")
          state = kMailTx
       else:
          send("535 Authentication failed\r\n")
       return
    send("530 Authentication required\r\n"); return

  if state == kMailTx:
    if cmd == "MAIL" and args startsWith "FROM:":
       tx.mailFrom = parsePath(args)
       send("250 OK\r\n"); return
    if cmd == "RCPT" and args startsWith "TO:":
       rcpt = parsePath(args)
       if !isLocalDomain(rcpt): send("550 No such user\r\n"); return
       tx.rcptList.add(rcpt)
       send("250 OK\r\n"); return
    if cmd == "DATA":
       if tx.mailFrom empty or tx.rcptList empty:
          send("503 Bad sequence of commands\r\n"); return
       send("354 End data with <CRLF>.<CRLF>\r\n")
       state = kData
       dataBuffer.clear()
       return
    send("500 Command unrecognized\r\n"); return

  if state == kData:
    # 这一阶段不要再按命令解析：每行都是正文
    # 收到单独一行 '.' 表示结束
    if line == ".\r\n":
       emlText = finalizeData(dataBuffer)  # 做 dot-unstuffing，确保 CRLF
       for rcpt in tx.rcptList:
         owner = mapRcptToLocalUserId(rcpt)
         path = storeEmlToFile(owner, emlText)
         meta = extractMinimalHeaders(emlText)
         insertMessageMeta(owner, tx.mailFrom, rcpt, meta.subject, meta.dateRaw, emlText.size, path)
       resetTransaction()
       state = kMailTx
       send("250 OK\r\n")
       return
    else:
       dataBuffer.append(line)
       return
```

### 12.2 POP3 Session（最小状态机）

状态：
- `kAuth` -> `kTxn` -> `kUpdate`

伪代码：

```text
onConnect:
  send("+OK simplemaild POP3\r\n")
  state = kAuth

onLine(line):
  cmd, args = parsePop3Command(line)

  if cmd == "QUIT":
    if state == kTxn:
      applyDeletes()  # 真删文件/真删记录 或 标记删除落库
    send("+OK Bye\r\n")
    close(); return

  if state == kAuth:
    if cmd == "USER": pendingUser = args; send("+OK\r\n"); return
    if cmd == "PASS":
      if authOk(pendingUser, args):
        authedUserId = loadUserId(pendingUser)
        mailbox = loadMailbox(authedUserId)  # 只加载 pop3_deleted=0
        send("+OK\r\n")
        state = kTxn
      else:
        send("-ERR auth failed\r\n")
      return
    send("-ERR\r\n"); return

  if state == kTxn:
    if cmd == "STAT":
      n = mailbox.countNotDeleted()
      bytes = mailbox.sumSize()
      send("+OK " + n + " " + bytes + "\r\n"); return

    if cmd == "LIST":
      if args empty:
        send("+OK\r\n")
        for i in 1..mailbox.size:
          if !mailbox[i].deleted: send(i + " " + mailbox[i].size + "\r\n")
        send(".\r\n")
      else:
        i = parseIndex(args)
        if invalid: send("-ERR\r\n") else send("+OK " + i + " " + mailbox[i].size + "\r\n")
      return

    if cmd == "RETR":
      i = parseIndex(args)
      msg = mailbox[i]
      eml = readFile(msg.path)
      send("+OK " + msg.size + " octets\r\n")
      send(eml)  # 原样
      if !eml endsWith CRLF: send("\r\n")
      send(".\r\n")
      return

    if cmd == "DELE":
      i = parseIndex(args)
      mailbox[i].deleted = true
      markDeletePending(msgId)
      send("+OK\r\n")
      return

    if cmd == "NOOP": send("+OK\r\n"); return
    send("-ERR\r\n"); return
```

---

## 13. 附录：手工联调脚本（nc / telnet）

> 目标：你在写完协议层后，先不用客户端，也能快速验证“服务端可用”。

### 13.1 SMTP 手工投递（AUTH PLAIN）

1) 生成 `AUTH PLAIN` 的 base64：格式为 `\0username\0password`

```bash
printf '\0alice\0your_password' | base64
```

2) 用 `nc` 连接并投递（注意 CRLF）：

```bash
SERVER=127.0.0.1
SMTP_PORT=2525
B64='上一步生成的base64字符串'

{
  printf 'EHLO local\r\n'
  printf 'AUTH PLAIN %s\r\n' "$B64"
  printf 'MAIL FROM:<alice@local.test>\r\n'
  printf 'RCPT TO:<bob@local.test>\r\n'
  printf 'DATA\r\n'
  printf 'From: Alice <alice@local.test>\r\n'
  printf 'To: Bob <bob@local.test>\r\n'
  printf 'Subject: hello\r\n'
  printf 'Date: Fri, 29 May 2026 12:00:00 +0800\r\n'
  printf '\r\n'
  printf 'this is a test mail\r\n'
  printf '.\r\n'
  printf 'QUIT\r\n'
} | nc -C "$SERVER" "$SMTP_PORT"
```

验收点：
- MySQL 的 `messages` 表新增一条记录
- `/var/lib/simplemail/mail/{user_id}/` 下新增一个 `.eml`

### 13.2 POP3 手工拉取（LIST/RETR）

```bash
SERVER=127.0.0.1
POP3_PORT=1110

{
  printf 'USER alice\r\n'
  printf 'PASS your_password\r\n'
  printf 'STAT\r\n'
  printf 'LIST\r\n'
  printf 'RETR 1\r\n'
  printf 'QUIT\r\n'
} | nc -C "$SERVER" "$POP3_PORT"
```

验收点：
- `LIST` 能看到编号与大小
- `RETR 1` 能拿到完整 `.eml` 原文，并以 `\r\n.\r\n` 结束

### 13.3 Foxmail 配置要点（阶段一）
- SMTP：服务器填你的 Ubuntu IP，端口 `2525`，关闭 SSL/TLS
- POP3：服务器填你的 Ubuntu IP，端口 `1110`，关闭 SSL/TLS
- 用户名/密码：使用你在 `users` 表里创建的账号

> 注意：阶段一不加 TLS 只适合内网测试。后续引入 TLS 后，Foxmail/你的 Qt 客户端配置也要改为 SSL/STARTTLS。

