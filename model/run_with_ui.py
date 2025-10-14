# python
# 文件: 'run_with_ui.py'（可选的启动脚本，小而清晰）
import threading
from main import EEGDataReceiver
from ui import StatusUI

def main():
    ui = StatusUI("SubEEG 监控面板")
    receiver = EEGDataReceiver(ui=ui)

    t = threading.Thread(target=receiver.start_server, daemon=True)
    t.start()

    ui.bind_on_close(receiver.stop_server)
    ui.run()

if __name__ == "__main__":
    main()
