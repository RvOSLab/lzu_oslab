/* lzuoslab.code-workspace */

let workspaceTemplate = {
    "extensions": {
        "recommendations": [
            "ms-vscode.cpptools",
            "zhwu95.riscv"
        ],
        "unwantedRecommendations": ["ms-vscode.cpptools-extension-pack"]
    },
    "folders": render([
        {
            "name": "{{oslab}}",
            "path": "os/{{oslab}}"
        },
        {
            "name": "doc",
            "path": "doc"
        }
    ]),
    "settings": {
        "C_Cpp.default.includePath": [
            "${workspaceFolder}/include",
            "${workspaceFolder}/"
        ],
        "C_Cpp.default.systemIncludePath": [],
        "C_Cpp.default.forcedInclude": [
            "${workspaceFolder}/../../vsc_diag_suppress.h"
        ],
        "C_Cpp.default.browse.limitSymbolsToIncludedHeaders": true,
        "C_Cpp.default.browse.path": ["${workspaceFolder}"],
        "C_Cpp.default.compilerPath": "riscv64-unknown-elf-gcc",
        "C_Cpp.default.cStandard": "gnu11",
        "C_Cpp.vcFormat.newLine.beforeElse": false,
        "C_Cpp.vcFormat.newLine.beforeOpenBrace.block": "sameLine",
        "C_Cpp.vcFormat.newLine.beforeOpenBrace.function": "sameLine",
        "C_Cpp.vcFormat.newLine.beforeOpenBrace.lambda": "sameLine",
        "C_Cpp.vcFormat.newLine.beforeOpenBrace.namespace": "sameLine",
        "C_Cpp.vcFormat.newLine.beforeOpenBrace.type": "sameLine",
        "C_Cpp.vcFormat.newLine.closeBraceSameLine.emptyFunction": true,
        "C_Cpp.vcFormat.newLine.closeBraceSameLine.emptyType": true,
        "C_Cpp.vcFormat.space.pointerReferenceAlignment": "right",
        "C_Cpp.vcFormat.space.preserveInInitializerList": false,
        "C_Cpp.vcFormat.newLine.beforeCatch": false,
        "files.associations": {
            "*.c": "c",
            "*.h": "c",
            "*.s": "riscv"
        },
        "search.exclude": {
            "**/*.md": true,
            "html": true,
            "Doxyfile": true,
            "kernel.map": true,
            ".clang-format": true
        },
        "search.useIgnoreFiles": true,
        "[c][riscv]": {
            "files.insertFinalNewline": true
        },
    },
    "tasks": {
        "version": "2.0.0",
        "tasks": render([
            {
                "type": "shell",
                "label": "{{oslab}}-make-clean",
                "command": "make clean",
                "options": {
                    "cwd": "${workspaceFolder:{{oslab}}}"
                },
                "presentation": {
                    "echo": false,
                    "reveal": "never",
                    "showReuseMessage": false,
                    "panel": "shared"
                }
            },
            {
                "type": "shell",
                "label": "{{oslab}}-make-all",
                "command": "make all",
                "dependsOn": "{{oslab}}-make-clean",
                "problemMatcher": "$gcc",
                "presentation": {
                    "echo": true,
                    "reveal": "always",
                    "panel": "shared",
                    "showReuseMessage": false
                },
                "options": {
                    "cwd": "${workspaceFolder:{{oslab}}}"
                }
            },
            {
                "type": "shell",
                "label": "{{oslab}}-qemu-debug",
                "command": ( // note: must in a single cmdline
                    "echo starting qemu... 1>&2 " + "&& " +
                    "qemu-system-riscv64 -machine virt -s -S -nographic " +
                    "-bios ${workspaceFolder:{{oslab}}}/tools/fw_jump.bin " +
                    "-device loader,file=kernel.img,addr=0x80200000"
                ),
                "isBackground": true,
                "presentation": {
                    "echo": true,
                    "reveal": "always",
                    "focus": true,
                    "panel": "shared",
                    "showReuseMessage": false
                },
                "dependsOn": "{{oslab}}-make-all",
                "options": {
                    "cwd": "${workspaceFolder:{{oslab}}}"
                },
                "problemMatcher": {
                    "pattern": {
                        "regexp": "."
                    },
                    "background": {  //note: must echo to stderr to emit it
                        "activeOnStart": true,
                        "beginsPattern": ".",
                        "endsPattern": "."
                    }
                }
            }
        ])
    },
    "launch": {
        "version": "0.2.0",
        "configurations": render([
            {
                "name": "调试{{oslab}}",
                "type": "cppdbg",
                "request": "launch",
                "miDebuggerServerAddress": "localhost:1234",
                "program": "${workspaceFolder:{{oslab}}}/kernel.bin",
                "sourceFileMap": {
                    "kernel.map": "${workspaceFolder:{{oslab}}}/kernel.map"
                },
                "MIMode": "gdb",
                "externalConsole": false,
                "miDebuggerPath": "gdb-multiarch",
                "internalConsoleOptions": "openOnSessionStart",
                "preLaunchTask": "{{oslab}}-qemu-debug",
                "cwd": "${workspaceFolder:{{oslab}}}",
                "setupCommands": [
                    {
                        "text": "-enable-pretty-printing",
                        "description": "Enable GDB pretty printing",
                        "ignoreFailures": true
                    },
                    {
                        "text": "set architecture riscv:rv64",
                        "description": "Set target architecture",
                        "ignoreFailures": true
                    }
                ],
                "postRemoteConnectCommands": [
                    // avoid access error when gdb read memory of breakpoint
                    {"text": "maintenance packet Qqemu.PhyMemMode:1"},
                    {"text": "tbreak main"},
                    {"text": "-break-commands 2 \"maintenance packet Qqemu.PhyMemMode:0\"" }
                ],
                "miDebuggerArgs": "-q"
            }
        ])
    }
}

function hasKey(obj, key) {
    return (JSON.stringify(obj).match(key)) ? true : false
}

function replaceAll(obj, src, dst) {
    return JSON.parse(JSON.stringify(obj).replace(src, dst))
}

function render(arrayOfTemplate) {
    const key = /{{oslab}}/g
    const oslab = [
        "lab1", "lab2", "lab3", "lab4",
        "lab5", "lab6", "lab7"
    ]
    const result = []
    for (const template of arrayOfTemplate) {
        if (hasKey(template, key)) {
            for (const lab of oslab) {
                result.push(
                    replaceAll(template, key, lab)
                )
            }
        } else {
            result.push(template)
        }
    }
    return result
}

// fix up lab1 fw_jump.bin path
workspaceTemplate = replaceAll(
    workspaceTemplate,
    "${workspaceFolder:lab1}/tools/fw_jump.bin",
    "${workspaceFolder:lab1}/fw_jump.bin"
)

/* vsc_diag_suppress.h */
const headerForVSC = `\
#ifdef __INTELLISENSE__
    #pragma diag_suppress 1118
    #pragma diag_suppress 1504
#endif
`

/* write to file */
const fs = require("fs")
fs.writeFileSync(
    "lzuoslab.code-workspace",
    JSON.stringify(workspaceTemplate, null, 4)
)
fs.writeFileSync(
    "vsc_diag_suppress.h",
    headerForVSC
)
