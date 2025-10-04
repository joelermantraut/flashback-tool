'''
This tool is a fast terminal tool that works like a flashback for some task that 
need to be remembered, in some time or any specific date.

When run, it could receive some arguments, like:
- date: the date to remember the task.
- time: number of minutes to remember the task.
- task: the task to remember.

The tool will remember the task for the time specified, or until the date specified. When the date is reached, the tark will be executed.
A notification will be sent to the user, showing the task message.

When added a new task, a new thread will be created to handle the task, so the main thread can continue receiving new tasks.
'''

import argparse
import json
import os
import time
import datetime
import uuid
import sys
import platform
import signal
import atexit

TASKS_FILE = os.path.expanduser("~/.flashback_tasks.json")

try:
    from plyer import notification
except ImportError:
    notification = None

def send_notification(msg, title="Flashback Reminder"):
    system = platform.system()
    if notification:
        notification.notify(title=title, message=msg, timeout=8)
    elif system == "Linux":
        os.system(f'notify-send "{title}" "{msg}"')
    elif system == "Darwin":
        os.system(f"""osascript -e 'display notification "{msg}" with title "{title}"'""")
    else:
        print(f"[Reminder] {title}: {msg}")


def load_tasks():
    if not os.path.exists(TASKS_FILE):
        return []
    try:
        with open(TASKS_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return []

def save_tasks(tasks):
    with open(TASKS_FILE, "w", encoding="utf-8") as f:
        json.dump(tasks, f, indent=2)

def add_task(text, target_time):
    tasks = load_tasks()
    task = {
        "id": str(uuid.uuid4()),
        "task": text,
        "target": target_time.isoformat()
    }
    tasks.append(task)
    save_tasks(tasks)
    print(f"Task added: '{text}' at {target_time}")


SHUTDOWN = False

def on_exit(*args):
    global SHUTDOWN
    SHUTDOWN = True
    send_notification("Flashback daemon stopped. Pending tasks are preserved.")

def run_daemon():
    print("Flashback daemon started...")
    send_notification("Flashback daemon started")

    while not SHUTDOWN:
        tasks = load_tasks()
        now = datetime.datetime.now()
        pending = []
        for t in tasks:
            target = datetime.datetime.fromisoformat(t["target"])
            if target <= now:
                send_notification(t["task"])
                print(f"Executed: {t['task']}")
            else:
                pending.append(t)
        if len(pending) != len(tasks):
            save_tasks(pending)
        time.sleep(1)

    print("Daemon stopped.")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--run", action="store_true", help="Run the main daemon")
    parser.add_argument("--task", type=str, help="Task description")
    parser.add_argument("--time", type=int, help="Minutes from now")
    parser.add_argument("--date", type=str, help="Absolute date YYYY-MM-DD HH:MM")
    args = parser.parse_args()

    if args.run:
        atexit.register(on_exit)
        signal.signal(signal.SIGINT, on_exit)
        signal.signal(signal.SIGTERM, on_exit)
        run_daemon()
        return

    if not args.task:
        print("You must specify --task with --time or --date")
        sys.exit(1)

    if args.time:
        target_time = datetime.datetime.now() + datetime.timedelta(minutes=args.time)
    elif args.date:
        try:
            target_time = datetime.datetime.strptime(args.date, "%Y-%m-%d %H:%M")
        except ValueError:
            print("Invalid date format. Use YYYY-MM-DD HH:MM")
            sys.exit(1)
    else:
        print("You must specify --time or --date with --task")
        sys.exit(1)

    add_task(args.task, target_time)


if __name__ == "__main__":
    main()
