# python
# 文件: 'ui.py'
import queue
import tkinter as tk
from tkinter import ttk

class StatusUI:
    def __init__(self, title="EEG Receiver Monitor"):
        self.root = tk.Tk()
        self.root.title(title)
        self.q = queue.Queue()

        # 变量
        self.tcp_var = tk.StringVar(value="未启动")
        self.phase_var = tk.StringVar(value="未开始")
        self.realtime_var = tk.StringVar(value="关闭")

        # 布局
        pad = {"padx": 10, "pady": 6}
        row = 0

        tk.Label(self.root, text="TCP状态:").grid(row=row, column=0, sticky="w", **pad)
        tk.Label(self.root, textvariable=self.tcp_var).grid(row=row, column=1, sticky="w", **pad)
        row += 1

        tk.Label(self.root, text="当前阶段:").grid(row=row, column=0, sticky="w", **pad)
        tk.Label(self.root, textvariable=self.phase_var).grid(row=row, column=1, sticky="w", **pad)
        row += 1

        tk.Label(self.root, text="训练进度:").grid(row=row, column=0, sticky="w", **pad)
        self.pbar = ttk.Progressbar(self.root, mode="indeterminate", length=240)
        self.pbar.grid(row=row, column=1, sticky="we", **pad)
        row += 1

        tk.Label(self.root, text="实时分类:").grid(row=row, column=0, sticky="w", **pad)
        tk.Label(self.root, textvariable=self.realtime_var).grid(row=row, column=1, sticky="w", **pad)
        row += 1

        for c in range(2):
            self.root.grid_columnconfigure(c, weight=1)

        # 轮询消息队列
        self.root.after(100, self._poll_queue)

        # 关闭回调
        self._on_close = None
        self.root.protocol("WM_DELETE_WINDOW", self._handle_close)

    # 线程安全：其他线程用 post 投递 UI 更新
    def post(self, func, *args, **kwargs):
        self.q.put((func, args, kwargs))

    def _poll_queue(self):
        try:
            while True:
                func, args, kwargs = self.q.get_nowait()
                try:
                    func(*args, **kwargs)
                except Exception:
                    pass
        except queue.Empty:
            pass
        self.root.after(100, self._poll_queue)

    # UI 更新方法
    def set_tcp(self, text: str):
        self.tcp_var.set(text)

    def set_phase(self, text: str):
        self.phase_var.set(text)

    def start_training(self):
        self.pbar.start(10)

    def stop_training(self, success: bool = True):
        self.pbar.stop()
        # 可根据 success 设置阶段提示
        if success:
            self.phase_var.set("阶段1已完成，模型就绪")
        else:
            self.phase_var.set("阶段1训练失败")

    def set_realtime(self, on: bool):
        self.realtime_var.set("开启" if on else "关闭")

    def bind_on_close(self, func):
        self._on_close = func

    def _handle_close(self):
        try:
            if self._on_close:
                self._on_close()
        finally:
            self.root.destroy()

    def run(self):
        self.root.mainloop()
