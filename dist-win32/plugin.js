exports.beforePlugin = "admin"
exports.apiRequired = 12
exports.description = "A plugin to work with HFS-ConsoleHide2Tray, allowing for a window to be hidden in the system tray using Named Pipes."
exports.version = 1
exports.repo = "pavelnil/HFS-ConsoleHide2Tray"
exports.preview = ["https://github.com/pavelnil/HFS-ConsoleHide2Tray/blob/main/screenshots/screenshot1.jpg?raw=true"]

exports.init = (api) => {
    const net = api.require('net');
    const child_process = api.require('child_process');
    const os = api.require('os');
    const fs = api.require('fs');
    
    let logEnabled = true;
    let server = null;

    const log = (message) => {
        if (logEnabled) api.log(`[ConsoleHide2Tray-agent] ${message}`);
    };

    const getMainWindow = () => {
        return new Promise((resolve) => {
            const psCode = `
Add-Type @"
    using System;
    using System.Runtime.InteropServices;

    public class Win32 {
        [DllImport("kernel32.dll")]
        public static extern IntPtr GetConsoleWindow();
        
        [DllImport("user32.dll")]
        public static extern IntPtr GetParent(IntPtr hWnd);
    }
"@

$consoleHwnd = [Win32]::GetConsoleWindow();
if ($consoleHwnd -eq [IntPtr]::Zero) {
    Write-Output "0x0"
    exit
}

$mainHwnd = [Win32]::GetParent($consoleHwnd);
if ($mainHwnd -eq [IntPtr]::Zero) {
    $mainHwnd = $consoleHwnd;
}

Write-Output "0x$($mainHwnd.ToString('X16'))"
            `;

            const buffer = Buffer.from(psCode, 'utf16le');
            const base64Script = buffer.toString('base64');
            
            child_process.exec(
                `powershell -ExecutionPolicy Bypass -EncodedCommand "${base64Script}"`,
                (err, stdout, stderr) => {
                    if (err) {
                        log(`Error getting main window: ${err.message}`);
                        resolve("0x0");
                        return;
                    }
                    
                    const hwnd = stdout.trim();
                    log(`Main HWND: ${hwnd}`);
                    resolve(hwnd);
                }
            );
        });
    };

    const createPipeServer = () => {
        const PIPE_NAME = "\\\\.\\pipe\\HFS_WINDOW_HWNDS";
        
        server = net.createServer((socket) => {
            socket.on('data', async (data) => {
                if (data.toString() === 'GET_HWNDS') {
                    try {
                        const mainHwnd = await getMainWindow();
                        socket.write(mainHwnd);
                        log("Sent main HWND to client");
                    } catch (e) {
                        log(`Error getting main window: ${e.message}`);
                        socket.write("0x0");
                    }
                }
            });
            
            socket.on('error', (err) => {
                log(`Pipe error: ${err.message}`);
            });
        });
        
        server.on('error', (err) => {
            log(`Server error: ${err.message}`);
        });
        
        server.listen(PIPE_NAME, () => {
            log(`Named pipe server listening on ${PIPE_NAME}`);
        });
    };

    createPipeServer();

    return {
        unload: () => {
            if (server) {
                server.close();
                log("Named pipe server closed");
            }
            log("Plugin unloaded");
        }
    };
};