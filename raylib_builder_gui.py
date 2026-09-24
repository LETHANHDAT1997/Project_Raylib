#!/usr/bin/env python3
"""
Raylib CMake Build GUI Tool
A modern Tkinter GUI to configure and build raylib with customizable options and output directory.
"""

import os
import sys
import shutil
import threading
import subprocess
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from tkinter.scrolledtext import ScrolledText

PLATFORMS = [
    "Desktop",
    "Win32",
    "Web",
    "WebRGFW",
    "Android",
    "Raspberry Pi",
    "DRM",
    "SDL",
    "RGFW",
    "Memory",
]

OPENGL_VERSIONS = [
    "OFF",
    "4.3",
    "3.3",
    "2.1",
    "1.1",
    "ES 2.0",
    "ES 3.0",
    "Software",
]

BUILD_TYPES = [
    "Release",
    "Debug",
    "RelWithDebInfo",
    "MinSizeRel",
]

GLFW_EXTERNAL_OPTIONS = [
    "OFF",
    "IF_POSSIBLE",
    "ON",
]


class RaylibBuilderApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Raylib CMake Builder")
        self.root.geometry("980x780")
        self.root.minsize(800, 600)

        # Determine default source directory (location of raylib)
        script_dir = os.path.dirname(os.path.abspath(__file__))
        if os.path.exists(os.path.join(script_dir, "CMakeLists.txt")):
            self.default_src_dir = script_dir
        else:
            self.default_src_dir = os.getcwd()

        self.current_process = None
        self.is_building = False

        self._setup_style()
        self._init_variables()
        self._create_widgets()
        self._update_cmd_preview()

    def _setup_style(self):
        self.style = ttk.Style()
        try:
            self.style.theme_use("clam")
        except Exception:
            pass

        # Configure colors and fonts
        self.style.configure(".", font=("Segoe UI", 9) if sys.platform == "win32" else ("Sans", 9))
        self.style.configure("Header.TLabel", font=("Sans", 12, "bold"))
        self.style.configure("Section.TLabelframe", padding=8)
        self.style.configure("Section.TLabelframe.Label", font=("Sans", 9, "bold"))
        self.style.configure("Primary.TButton", font=("Sans", 9, "bold"))

    def _init_variables(self):
        # Directories
        self.var_src_dir = tk.StringVar(value=self.default_src_dir)
        self.var_build_dir = tk.StringVar(value=os.path.join(self.default_src_dir, "build"))
        self.var_install_dir = tk.StringVar(value="")

        # Core options
        self.var_platform = tk.StringVar(value="Desktop")
        self.var_opengl = tk.StringVar(value="OFF")
        self.var_build_type = tk.StringVar(value="Release")
        self.var_shared_libs = tk.BooleanVar(value=False)
        self.var_build_examples = tk.BooleanVar(value=False)
        self.var_use_audio = tk.BooleanVar(value=True)
        self.var_with_pic = tk.BooleanVar(value=False)

        # GLFW options
        self.var_external_glfw = tk.StringVar(value="OFF")
        self.var_glfw_x11 = tk.BooleanVar(value=True)
        self.var_glfw_wayland = tk.BooleanVar(value=False)

        # Sanitizers & Debug
        self.var_asan = tk.BooleanVar(value=False)
        self.var_ubsan = tk.BooleanVar(value=False)
        self.var_msan = tk.BooleanVar(value=False)

        # Build options
        cpu_count = os.cpu_count() or 4
        self.var_parallel_jobs = tk.IntVar(value=cpu_count)
        self.var_generator = tk.StringVar(value="Default")
        self.var_extra_flags = tk.StringVar(value="")

        # Trace variables to auto update command preview
        for var in [
            self.var_src_dir, self.var_build_dir, self.var_install_dir,
            self.var_platform, self.var_opengl, self.var_build_type,
            self.var_shared_libs, self.var_build_examples, self.var_use_audio,
            self.var_with_pic, self.var_external_glfw, self.var_glfw_x11,
            self.var_glfw_wayland, self.var_asan, self.var_ubsan, self.var_msan,
            self.var_parallel_jobs, self.var_generator, self.var_extra_flags
        ]:
            var.trace_add("write", lambda *_: self._update_cmd_preview())

    def _create_widgets(self):
        # Top banner
        header_frame = ttk.Frame(self.root, padding=(12, 10, 12, 6))
        header_frame.pack(fill=tk.X)

        title_lbl = ttk.Label(header_frame, text="⚡ Raylib CMake Build Manager", style="Header.TLabel")
        title_lbl.pack(side=tk.LEFT)

        subtitle_lbl = ttk.Label(header_frame, text="Tùy chỉnh cấu hình & build raylib trực quan", foreground="#666")
        subtitle_lbl.pack(side=tk.LEFT, padx=(10, 0))

        # Main Paned / Vertical Split (Top: Configuration Notebook, Bottom: Log)
        paned = ttk.PanedWindow(self.root, orient=tk.VERTICAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # Top scrollable / frame for config
        config_parent = ttk.Frame(paned)
        paned.add(config_parent, weight=3)

        # Directory Configuration Frame
        dir_frame = ttk.LabelFrame(config_parent, text="📁 Thư Mục Làm Việc (Directories)", style="Section.TLabelframe")
        dir_frame.pack(fill=tk.X, padx=5, pady=(0, 6))

        # Source dir
        ttk.Label(dir_frame, text="Mã nguồn (Source -S):", width=22, anchor=tk.W).grid(row=0, column=0, padx=5, pady=3, sticky=tk.W)
        src_entry = ttk.Entry(dir_frame, textvariable=self.var_src_dir)
        src_entry.grid(row=0, column=1, padx=5, pady=3, sticky=tk.EW)
        ttk.Button(dir_frame, text="Duyệt...", command=self._browse_src_dir, width=10).grid(row=0, column=2, padx=5, pady=3)

        # Build output dir
        ttk.Label(dir_frame, text="Thư mục Build (Output -B):", width=22, anchor=tk.W).grid(row=1, column=0, padx=5, pady=3, sticky=tk.W)
        build_entry = ttk.Entry(dir_frame, textvariable=self.var_build_dir)
        build_entry.grid(row=1, column=1, padx=5, pady=3, sticky=tk.EW)
        
        btn_box = ttk.Frame(dir_frame)
        btn_box.grid(row=1, column=2, padx=5, pady=3, sticky=tk.E)
        ttk.Button(btn_box, text="Duyệt...", command=self._browse_build_dir, width=7).pack(side=tk.LEFT, padx=(0, 3))
        ttk.Button(btn_box, text="Xóa Build", command=self._clean_build_dir, width=9).pack(side=tk.LEFT)

        dir_frame.columnconfigure(1, weight=1)

        # Notebook for detailed configuration tabs
        notebook = ttk.Notebook(config_parent)
        notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=0)

        # Tab 1: Cấu hình chính (Core Options)
        tab_core = ttk.Frame(notebook, padding=10)
        notebook.add(tab_core, text="⚙️ Cấu Hình Chính")
        self._build_tab_core(tab_core)

        # Tab 2: GLFW & Hệ Thống (GLFW & System)
        tab_system = ttk.Frame(notebook, padding=10)
        notebook.add(tab_system, text="🖥️ GLFW & Hiển Thị")
        self._build_tab_system(tab_system)

        # Tab 3: Debug & Nâng Cao (Advanced)
        tab_advanced = ttk.Frame(notebook, padding=10)
        notebook.add(tab_advanced, text="🛠️ Debug & Tùy Chọn Thêm")
        self._build_tab_advanced(tab_advanced)

        # Command Preview Frame
        cmd_frame = ttk.LabelFrame(config_parent, text="📝 Lệnh CMake Sẽ Thực Thi (Preview)", style="Section.TLabelframe")
        cmd_frame.pack(fill=tk.X, padx=5, pady=(6, 2))

        self.cmd_preview_text = tk.Text(cmd_frame, height=2, wrap=tk.CHAR, bg="#1e1e1e", fg="#dcdcdc", insertbackground="white", font=("Consolas", 9))
        self.cmd_preview_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(4, 6), pady=4)
        
        preview_btns = ttk.Frame(cmd_frame)
        preview_btns.pack(side=tk.RIGHT, padx=4, pady=4)
        ttk.Button(preview_btns, text="📋 Sao chép", command=self._copy_cmd, width=11).pack(pady=2)

        # Bottom Frame: Output Log & Action Buttons
        bottom_frame = ttk.Frame(paned)
        paned.add(bottom_frame, weight=4)

        # Action Buttons bar
        actions_bar = ttk.Frame(bottom_frame, padding=(5, 4, 5, 4))
        actions_bar.pack(fill=tk.X)

        self.btn_configure = ttk.Button(actions_bar, text="1. Configure (CMake)", command=self._on_click_configure, width=19)
        self.btn_configure.pack(side=tk.LEFT, padx=3)

        self.btn_build = ttk.Button(actions_bar, text="2. Build", command=self._on_click_build, width=15)
        self.btn_build.pack(side=tk.LEFT, padx=3)

        self.btn_both = ttk.Button(actions_bar, text="▶ Configure & Build Ngay", command=self._on_click_both, width=24, style="Primary.TButton")
        self.btn_both.pack(side=tk.LEFT, padx=6)

        self.btn_stop = ttk.Button(actions_bar, text="⏹ Dừng (Stop)", command=self._on_click_stop, state=tk.DISABLED, width=14)
        self.btn_stop.pack(side=tk.LEFT, padx=3)

        ttk.Button(actions_bar, text="Dọn Log", command=self._clear_log, width=10).pack(side=tk.RIGHT, padx=3)

        # Log Display
        log_frame = ttk.LabelFrame(bottom_frame, text="📜 Output Log", style="Section.TLabelframe")
        log_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=2)

        self.log_text = ScrolledText(log_frame, wrap=tk.CHAR, bg="#181818", fg="#e0e0e0", insertbackground="white", font=("Consolas", 9))
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

        # Tags for colored log
        self.log_text.tag_config("info", foreground="#61afef")
        self.log_text.tag_config("success", foreground="#98c379", font=("Consolas", 9, "bold"))
        self.log_text.tag_config("error", foreground="#e06c75", font=("Consolas", 9, "bold"))
        self.log_text.tag_config("warning", foreground="#e5c07b")
        self.log_text.tag_config("cmd", foreground="#c678dd", font=("Consolas", 9, "bold"))

        # Status Bar
        self.status_bar = ttk.Label(self.root, text="Sẵn sàng.", relief=tk.SUNKEN, anchor=tk.W, padding=(6, 2))
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    def _build_tab_core(self, parent):
        grid_frame = ttk.Frame(parent)
        grid_frame.pack(fill=tk.BOTH, expand=True)

        # Platform
        ttk.Label(grid_frame, text="Nền tảng (PLATFORM):", font=("Sans", 9, "bold")).grid(row=0, column=0, sticky=tk.W, padx=5, pady=5)
        platform_cb = ttk.Combobox(grid_frame, textvariable=self.var_platform, values=PLATFORMS, state="readonly", width=18)
        platform_cb.grid(row=0, column=1, sticky=tk.W, padx=5, pady=5)
        ttk.Label(grid_frame, text="(Desktop, Web, Android, DRM, SDL, RGFW...)", foreground="#666").grid(row=0, column=2, sticky=tk.W, padx=5)

        # OpenGL Version
        ttk.Label(grid_frame, text="Phiên bản OpenGL (OPENGL_VERSION):", font=("Sans", 9, "bold")).grid(row=1, column=0, sticky=tk.W, padx=5, pady=5)
        opengl_cb = ttk.Combobox(grid_frame, textvariable=self.var_opengl, values=OPENGL_VERSIONS, state="readonly", width=18)
        opengl_cb.grid(row=1, column=1, sticky=tk.W, padx=5, pady=5)
        ttk.Label(grid_frame, text="('OFF' = Tự động nhận diện)", foreground="#666").grid(row=1, column=2, sticky=tk.W, padx=5)

        # Build Type
        ttk.Label(grid_frame, text="Kiểu Build (CMAKE_BUILD_TYPE):", font=("Sans", 9, "bold")).grid(row=2, column=0, sticky=tk.W, padx=5, pady=5)
        btype_cb = ttk.Combobox(grid_frame, textvariable=self.var_build_type, values=BUILD_TYPES, state="readonly", width=18)
        btype_cb.grid(row=2, column=1, sticky=tk.W, padx=5, pady=5)
        ttk.Label(grid_frame, text="(Release tối ưu tốc độ, Debug có thông tin gỡ lỗi)", foreground="#666").grid(row=2, column=2, sticky=tk.W, padx=5)

        # Separator
        ttk.Separator(grid_frame, orient=tk.HORIZONTAL).grid(row=3, column=0, columnspan=3, sticky=tk.EW, pady=8)

        # Checkboxes for common flags
        chk_frame = ttk.Frame(grid_frame)
        chk_frame.grid(row=4, column=0, columnspan=3, sticky=tk.W, padx=5, pady=3)

        ttk.Checkbutton(chk_frame, text="Build dạng Shared Library (.so / .dll) (BUILD_SHARED_LIBS)", variable=self.var_shared_libs).pack(anchor=tk.W, pady=2)
        ttk.Checkbutton(chk_frame, text="Build Examples đi kèm (BUILD_EXAMPLES)", variable=self.var_build_examples).pack(anchor=tk.W, pady=2)
        ttk.Checkbutton(chk_frame, text="Kích hoạt Module Âm thanh (USE_AUDIO)", variable=self.var_use_audio).pack(anchor=tk.W, pady=2)
        ttk.Checkbutton(chk_frame, text="Bật PIC cho thư viện tĩnh (WITH_PIC)", variable=self.var_with_pic).pack(anchor=tk.W, pady=2)

    def _build_tab_system(self, parent):
        grid_frame = ttk.Frame(parent)
        grid_frame.pack(fill=tk.BOTH, expand=True)

        # External GLFW
        ttk.Label(grid_frame, text="Dùng GLFW ngoài (USE_EXTERNAL_GLFW):", font=("Sans", 9, "bold")).grid(row=0, column=0, sticky=tk.W, padx=5, pady=5)
        glfw_ext_cb = ttk.Combobox(grid_frame, textvariable=self.var_external_glfw, values=GLFW_EXTERNAL_OPTIONS, state="readonly", width=18)
        glfw_ext_cb.grid(row=0, column=1, sticky=tk.W, padx=5, pady=5)
        ttk.Label(grid_frame, text="(OFF: dùng GLFW nhúng kèm raylib)", foreground="#666").grid(row=0, column=2, sticky=tk.W, padx=5)

        ttk.Separator(grid_frame, orient=tk.HORIZONTAL).grid(row=1, column=0, columnspan=3, sticky=tk.EW, pady=8)

        # Linux Display backends
        ttk.Label(grid_frame, text="Cấu hình hiển thị Linux (X11 / Wayland):", font=("Sans", 9, "bold")).grid(row=2, column=0, columnspan=3, sticky=tk.W, padx=5, pady=3)

        linux_opts = ttk.Frame(grid_frame)
        linux_opts.grid(row=3, column=0, columnspan=3, sticky=tk.W, padx=5, pady=3)
        ttk.Checkbutton(linux_opts, text="GLFW hỗ trợ X11 (GLFW_BUILD_X11)", variable=self.var_glfw_x11).pack(side=tk.LEFT, padx=(0, 20))
        ttk.Checkbutton(linux_opts, text="GLFW hỗ trợ Wayland (GLFW_BUILD_WAYLAND)", variable=self.var_glfw_wayland).pack(side=tk.LEFT)

        ttk.Label(grid_frame, text="* Lưu ý trên Linux: Phải bật ít nhất một trong hai (X11 hoặc Wayland).", foreground="#888", font=("Sans", 8)).grid(row=4, column=0, columnspan=3, sticky=tk.W, padx=5, pady=(5, 0))

    def _build_tab_advanced(self, parent):
        grid_frame = ttk.Frame(parent)
        grid_frame.pack(fill=tk.BOTH, expand=True)

        # Sanitizers
        ttk.Label(grid_frame, text="Sanitizers (Kiểm tra lỗi bộ nhớ khi Debug):", font=("Sans", 9, "bold")).grid(row=0, column=0, columnspan=3, sticky=tk.W, padx=5, pady=4)
        san_frame = ttk.Frame(grid_frame)
        san_frame.grid(row=1, column=0, columnspan=3, sticky=tk.W, padx=5, pady=2)
        ttk.Checkbutton(san_frame, text="AddressSanitizer (ENABLE_ASAN)", variable=self.var_asan).pack(side=tk.LEFT, padx=(0, 15))
        ttk.Checkbutton(san_frame, text="UndefinedBehavior (ENABLE_UBSAN)", variable=self.var_ubsan).pack(side=tk.LEFT, padx=(0, 15))
        ttk.Checkbutton(san_frame, text="MemorySanitizer (ENABLE_MSAN)", variable=self.var_msan).pack(side=tk.LEFT)

        ttk.Separator(grid_frame, orient=tk.HORIZONTAL).grid(row=2, column=0, columnspan=3, sticky=tk.EW, pady=8)

        # Parallel build jobs
        ttk.Label(grid_frame, text="Số luồng CPU build (-j):", font=("Sans", 9, "bold")).grid(row=3, column=0, sticky=tk.W, padx=5, pady=4)
        cpu_box = ttk.Spinbox(grid_frame, from_=1, to=128, textvariable=self.var_parallel_jobs, width=8)
        cpu_box.grid(row=3, column=1, sticky=tk.W, padx=5, pady=4)

        # CMake Generator
        ttk.Label(grid_frame, text="CMake Generator (-G):", font=("Sans", 9, "bold")).grid(row=4, column=0, sticky=tk.W, padx=5, pady=4)
        gen_combo = ttk.Combobox(grid_frame, textvariable=self.var_generator, values=["Default", "Ninja", "Unix Makefiles"], width=18, state="readonly")
        gen_combo.grid(row=4, column=1, sticky=tk.W, padx=5, pady=4)

        # Extra flags
        ttk.Label(grid_frame, text="Cờ CMake bổ sung (Extra flags):", font=("Sans", 9, "bold")).grid(row=5, column=0, sticky=tk.W, padx=5, pady=4)
        extra_entry = ttk.Entry(grid_frame, textvariable=self.var_extra_flags, width=40)
        extra_entry.grid(row=5, column=1, columnspan=2, sticky=tk.W, padx=5, pady=4)
        ttk.Label(grid_frame, text="Ví dụ: -DCMAKE_EXPORT_COMPILE_COMMANDS=ON", foreground="#888", font=("Sans", 8)).grid(row=6, column=1, columnspan=2, sticky=tk.W, padx=5)

    def _browse_src_dir(self):
        selected = filedialog.askdirectory(initialdir=self.var_src_dir.get(), title="Chọn thư mục chứa mã nguồn raylib")
        if selected:
            self.var_src_dir.set(selected)
            # Default build dir updates to inside src dir
            self.var_build_dir.set(os.path.join(selected, "build"))

    def _browse_build_dir(self):
        selected = filedialog.askdirectory(initialdir=self.var_build_dir.get(), title="Chọn thư mục build output")
        if selected:
            self.var_build_dir.set(selected)

    def _clean_build_dir(self):
        bdir = self.var_build_dir.get().strip()
        if not bdir:
            messagebox.showwarning("Cảnh báo", "Chưa nhập thư mục build.")
            return

        if not os.path.exists(bdir):
            messagebox.showinfo("Thông báo", f"Thư mục build '{bdir}' chưa tồn tại.")
            return

        confirm = messagebox.askyesno("Xác nhận xóa", f"Bạn có chắc chắn muốn xóa toàn bộ nội dung trong:\n{bdir}?")
        if confirm:
            try:
                shutil.rmtree(bdir)
                self._log_text(f"[Dọn dẹp] Đã xóa thư mục: {bdir}\n", "warning")
                self.status_bar.config(text=f"Đã dọn dẹp thư mục: {bdir}")
            except Exception as e:
                messagebox.showerror("Lỗi", f"Không thể xóa thư mục:\n{str(e)}")

    def _get_cmake_configure_cmd(self):
        src = self.var_src_dir.get().strip()
        build = self.var_build_dir.get().strip()

        cmd = ["cmake", "-B", build, "-S", src]

        # Generator
        gen = self.var_generator.get()
        if gen != "Default":
            cmd.extend(["-G", gen])

        # Build type
        cmd.append(f"-DCMAKE_BUILD_TYPE={self.var_build_type.get()}")

        # Platform & OpenGL
        cmd.append(f"-DPLATFORM={self.var_platform.get()}")
        if self.var_opengl.get() != "OFF":
            cmd.append(f"-DOPENGL_VERSION={self.var_opengl.get()}")
        else:
            cmd.append("-DOPENGL_VERSION=OFF")

        # Flags
        cmd.append(f"-DBUILD_SHARED_LIBS={'ON' if self.var_shared_libs.get() else 'OFF'}")
        cmd.append(f"-DBUILD_EXAMPLES={'ON' if self.var_build_examples.get() else 'OFF'}")
        cmd.append(f"-DUSE_AUDIO={'ON' if self.var_use_audio.get() else 'OFF'}")
        cmd.append(f"-DWITH_PIC={'ON' if self.var_with_pic.get() else 'OFF'}")

        # GLFW
        cmd.append(f"-DUSE_EXTERNAL_GLFW={self.var_external_glfw.get()}")
        cmd.append(f"-DGLFW_BUILD_X11={'ON' if self.var_glfw_x11.get() else 'OFF'}")
        cmd.append(f"-DGLFW_BUILD_WAYLAND={'ON' if self.var_glfw_wayland.get() else 'OFF'}")

        # Sanitizers
        if self.var_asan.get():
            cmd.append("-DENABLE_ASAN=ON")
        if self.var_ubsan.get():
            cmd.append("-DENABLE_UBSAN=ON")
        if self.var_msan.get():
            cmd.append("-DENABLE_MSAN=ON")

        # Extra flags
        extra = self.var_extra_flags.get().strip()
        if extra:
            import shlex
            cmd.extend(shlex.split(extra))

        return cmd

    def _get_cmake_build_cmd(self):
        build = self.var_build_dir.get().strip()
        jobs = self.var_parallel_jobs.get()
        return ["cmake", "--build", build, "-j", str(jobs)]

    def _update_cmd_preview(self):
        conf_cmd = " ".join(self._get_cmake_configure_cmd())
        build_cmd = " ".join(self._get_cmake_build_cmd())
        full_str = f"{conf_cmd}\n{build_cmd}"
        
        self.cmd_preview_text.config(state=tk.NORMAL)
        self.cmd_preview_text.delete("1.0", tk.END)
        self.cmd_preview_text.insert(tk.END, full_str)
        self.cmd_preview_text.config(state=tk.DISABLED)

    def _copy_cmd(self):
        text = self.cmd_preview_text.get("1.0", tk.END).strip()
        self.root.clipboard_clear()
        self.root.clipboard_append(text)
        self.status_bar.config(text="Đã copy lệnh vào Clipboard!")

    def _clear_log(self):
        self.log_text.delete("1.0", tk.END)

    def _log_text(self, text, tag=None):
        self.log_text.insert(tk.END, text, tag)
        self.log_text.see(tk.END)

    def _set_ui_state(self, is_running):
        self.is_building = is_running
        state = tk.DISABLED if is_running else tk.NORMAL
        self.btn_configure.config(state=state)
        self.btn_build.config(state=state)
        self.btn_both.config(state=state)
        self.btn_stop.config(state=tk.NORMAL if is_running else tk.DISABLED)

    def _run_process_async(self, commands):
        """Run a list of commands sequentially in a separate thread."""
        def worker():
            for cmd in commands:
                cmd_str = " ".join(cmd)
                self.root.after(0, self._log_text, f"\n$ {cmd_str}\n", "cmd")
                self.root.after(0, self.status_bar.config, {"text": f"Đang chạy: {cmd_str[:60]}..."})

                try:
                    self.current_process = subprocess.Popen(
                        cmd,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT,
                        text=True,
                        bufsize=1,
                        universal_newlines=True
                    )

                    for line in iter(self.current_process.stdout.readline, ""):
                        if not line:
                            break
                        # Detect errors / warnings for coloring
                        tag = None
                        lower_line = line.lower()
                        if "error:" in lower_line or "fatal error" in lower_line or "cmake error" in lower_line:
                            tag = "error"
                        elif "warning:" in lower_line:
                            tag = "warning"
                        elif "built target" in lower_line or "[100%]" in line or "build files have been written" in lower_line:
                            tag = "success"

                        self.root.after(0, self._log_text, line, tag)

                    self.current_process.wait()
                    code = self.current_process.returncode

                    if code != 0:
                        self.root.after(0, self._log_text, f"\n❌ Quá trình kết thúc với mã lỗi {code}\n", "error")
                        self.root.after(0, self.status_bar.config, {"text": f"Thất bại (Exit code: {code})"})
                        self.root.after(0, self._set_ui_state, False)
                        return

                except Exception as e:
                    self.root.after(0, self._log_text, f"\n❌ Lỗi thực thi: {str(e)}\n", "error")
                    self.root.after(0, self.status_bar.config, {"text": f"Lỗi: {str(e)}"})
                    self.root.after(0, self._set_ui_state, False)
                    return

            self.root.after(0, self._log_text, "\n🎉 HOÀN THÀNH THÀNH CÔNG!\n", "success")
            self.root.after(0, self.status_bar.config, {"text": "Hoàn thành thành công!"})
            self.root.after(0, self._set_ui_state, False)

        self._set_ui_state(True)
        thread = threading.Thread(target=worker, daemon=True)
        thread.start()

    def _validate_src(self):
        src = self.var_src_dir.get().strip()
        if not src or not os.path.exists(src):
            messagebox.showerror("Lỗi", "Thư mục mã nguồn không tồn tại.")
            return False
        if not os.path.exists(os.path.join(src, "CMakeLists.txt")):
            messagebox.showwarning("Cảnh báo", "Không tìm thấy CMakeLists.txt trong thư mục nguồn đã chọn.")
            return False
        return True

    def _on_click_configure(self):
        if not self._validate_src():
            return
        cmd = self._get_cmake_configure_cmd()
        self._run_process_async([cmd])

    def _on_click_build(self):
        build_dir = self.var_build_dir.get().strip()
        if not os.path.exists(build_dir):
            messagebox.showwarning("Chưa Configure", "Thư mục build chưa tồn tại. Vui lòng chạy Configure trước.")
            return
        cmd = self._get_cmake_build_cmd()
        # print(cmd)
        self._run_process_async([cmd])

    def _on_click_both(self):
        if not self._validate_src():
            return
        cmd_conf = self._get_cmake_configure_cmd()
        cmd_build = self._get_cmake_build_cmd()
        self._run_process_async([cmd_conf, cmd_build])

    def _on_click_stop(self):
        if self.current_process and self.current_process.poll() is None:
            try:
                self.current_process.terminate()
                self._log_text("\n🛑 Đã gửi tín hiệu dừng tới tiến trình...\n", "warning")
                self.status_bar.config(text="Đã dừng tiến trình.")
            except Exception as e:
                self._log_text(f"Không thể dừng tiến trình: {e}\n", "error")


def main():
    root = tk.Tk()
    app = RaylibBuilderApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
