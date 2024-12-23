import os
import re
import subprocess
import sys
import platform
from pathlib import Path
from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


def get_version():
    toml_file = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'pyproject.toml')
    with open(toml_file, 'r') as f:
        toml_content = f.read()
    return re.search(r"^version\s=\s\"(.*?)\"$", toml_content, re.MULTILINE)[1]


class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = "") -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        ext_dir = ext_fullpath.parent.resolve()

        cmake_args = [
            "-DCANDLE_API_BUILD_PYTHON=ON",
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={ext_dir}{os.sep}candle_api{os.sep}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release"
        ]

        if platform.system() == 'Linux':
            libc, libc_ver = platform.libc_ver()
            if libc == 'glibc':
                libc_ver = tuple(map(int, libc_ver.split('.')))
                if libc_ver < (2, 28):
                    cmake_args.append("-DCANDLE_API_TINYCTHREADS=ON")

        build_args = []

        if self.compiler.compiler_type == "msvc":
            cmake_args.append(f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE={ext_dir}{os.sep}candle_api")
            build_args += ["--config", "Release"]

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)

        subprocess.run(
            ["cmake", ext.sourcedir] + cmake_args, cwd=build_temp, check=True
        )
        subprocess.run(
            ["cmake", "--build", "."] + build_args, cwd=build_temp, check=True
        )


setup(
    name="candle_api",
    version=get_version(),
    ext_modules=[CMakeExtension("candle_api")],
    cmdclass={"build_ext": CMakeBuild}
)
