import sqlite3
import os
import json

DB_PATH = r"D:\WorkSpace\ZCode\260715\ChronoChat\backend\data\chronochat.db"
CHARS_DIR = r"D:\WorkSpace\ZCode\260715\ChronoChat\data\characters"

conn = sqlite3.connect(DB_PATH)
cur = conn.cursor()

# Get existing IDs
cur.execute("SELECT id FROM characters")
existing = set(row[0] for row in cur.fetchall())

import time
now_ms = int(time.time() * 1000)

recovered = []
for dir_name in os.listdir(CHARS_DIR):
    dir_path = os.path.join(CHARS_DIR, dir_name)
    if not os.path.isdir(dir_path):
        continue
    
    if dir_name in existing:
        print(f"  SKIP (exists): {dir_name}")
        continue
    
    # Try to extract name from IDENTITY.md
    name = dir_name[:8] + "..."
    identity_path = os.path.join(dir_path, "IDENTITY.md")
    if os.path.exists(identity_path):
        with open(identity_path, "r", encoding="utf-8") as f:
            for line in f:
                for prefix in ["Name:", "姓名：", "**Name:**", "名字："]:
                    if prefix in line:
                        candidate = line.split(prefix, 1)[1].strip().strip("*").strip()
                        if candidate and len(candidate) < 50:
                            name = candidate
                            break
                else:
                    continue
                break
    
    print(f"  RECOVER: {dir_name} -> {name}")
    
    cur.execute("""
        INSERT INTO characters(id,name,gender,age,birthday,mbti,personality_signature,
            avatar_path,story_text,story_images,user_description,
            text_api_base_url,text_api_key,text_model,
            multimodal_api_base_url,multimodal_api_key,multimodal_model,
            last_active_date,created_at,updated_at)
        VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    """, (
        dir_name, name, "", 0, "", "", "[Recovered]",
        "", "", "[]", "",
        "", "", "",
        "", "", "",
        "", now_ms, now_ms
    ))
    recovered.append((dir_name, name))

conn.commit()
conn.close()

print(f"\nRecovered {len(recovered)} characters:")
for rid, rname in recovered:
    print(f"  {rid} -> {rname}")
