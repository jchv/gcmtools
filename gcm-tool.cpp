/*
 * gcm-info.cpp - (C) 2012-2013 jchadwick <johnwchadwick@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <nall/nall.hpp>
#include <nall/string.hpp>
#include <phoenix/phoenix.hpp>
#include <gcm.hpp>

using namespace nall;
using namespace phoenix;

const string Title = "GCM Tool";

void extractDir(gamecube::fst::entry root, string target) {
	directory::create(target);

	for(auto node : root.children) {
		if(node.children.empty()) {
			unsigned size = node.data.len;
			uint8_t *data = new uint8_t[size];
			node.data.read(data);

			file::write({target, "/", node.name}, data, size);
			delete[] data;
		} else	extractDir(node, {target, "/", node.name});
	}
}

bool unpack(string inFile, string outDir) {
	gamecube::gcm iso;
	string root = {outDir, "/root"};
	string sys = {outDir, "/sys"};

	if(!iso.open(new filestream(inFile)))
		return false;

	extractDir(iso.filesystem.root, root);

	directory::create(sys);
	filestream bootfile({sys, "/boot.bin"}, file::mode::write);
	filestream bi2file({sys, "/bi2.bin"}, file::mode::write);
	filestream appldrfile({sys, "/apploader.img"}, file::mode::write);
	filestream binaryfile({sys, "/main.dol"}, file::mode::write);
	filestream fstfile({sys, "/fst.bin"}, file::mode::write);

	iso.writeBootHeader(&bootfile);
	iso.writeBi2Header(&bi2file);
	iso.appldr.write(&appldrfile);
	iso.binary.write(&binaryfile);
	iso.filesystem.write(&fstfile, false);

	return true;
}

void archiveDir(gamecube::fst::entry &root, string source, string subdir="") {
	lstring files = directory::files({source, subdir});
	lstring folders = directory::folders({source, subdir});

	for(auto &name : files) {
		gamecube::fst::entry file;
		file.name = name;
		file.data = gamecube::fst::dataref({source, subdir, "/", name});
		root.children.append(file);
	}

	for(auto &name : folders) {
		gamecube::fst::entry dir;
		dir.name = name.rtrim("/");
		archiveDir(dir, source, {subdir, "/", name});

		root.children.append(dir);
	}
}

bool repack(string inDir, string outFile) {
	gamecube::gcm iso;
	string root = {inDir, "/root"};
	string sys = {inDir, "/sys"};

	archiveDir(iso.filesystem.root, root);

	filestream bootfile({sys, "/boot.bin"}, file::mode::read);
	iso.readBootHeader(&bootfile);
	filestream bi2file({sys, "/bi2.bin"}, file::mode::read);
	iso.readBi2Header(&bi2file);
	filestream appldrfile({sys, "/apploader.img"}, file::mode::read);
	iso.appldr.read(&appldrfile);
	filestream binaryfile({sys, "/main.dol"}, file::mode::read);
	iso.binary.read(&binaryfile);

	filestream isofile(outFile, file::mode::write);
	iso.write(&isofile);

	return true;
}

struct Application : Window {
	HorizontalLayout layout;
	Label label;
	Button unpackButton;
	Button repackButton;
	Button helpButton;

	Application(int argc, char **argv) {
		string basepath = dir(realpath(argv[0]));

		setTitle(Title);
		setResizable(false);

		label.setText("Actions: ");
		layout.append(label, { 0, 0 }, 5);
		unpackButton.setText("unpack");
		layout.append(unpackButton, { 0, 0 }, 5);
		repackButton.setText("repack");
		layout.append(repackButton, { 0, 0 }, 5);
		helpButton.setText("help");
		layout.append(helpButton, { 0, 0 }, 5);
		append(layout);

		unpackButton.onActivate = [&]() {
			string inFile, outDir;
			inFile = DialogWindow::fileOpen(*this, "", "Gamecube ISO (*.iso)", "Gamecube GCM (*.gcm)");
			if(inFile.empty()) return;
			outDir = DialogWindow::folderSelect(*this, dir(inFile));
			if(outDir.empty()) return;

			setBusy(true);
			unpack(inFile, outDir);
			setBusy(false);
		};
		repackButton.onActivate = [&]() {
			string inDir, outFile;
			inDir = DialogWindow::folderSelect(*this, "");
			if(inDir.empty()) return;
			outFile = DialogWindow::fileSave(*this, "", "Gamecube ISO (*.iso)", "Gamecube GCM (*.gcm)");
			if(outFile.empty()) return;

			setBusy(true);
			repack(inDir, outFile);
			setBusy(false);
		};
		helpButton.onActivate = [&]() {
			string helpText = {
				Title, " by jchadwick <johnwchadwick@gmail.com>\n",
				"--\n"
				"Usage:\n"
				"- unpack: dumps data structures from an image, to a directory\n",
				"- repack: constructs an image, from a directory containing a dump\n",
				"--\n"
				"When unpacking, you should select an EMPTY DIRECTORY\n"
				"or create a new directory."
			};
			MessageWindow::information(*this, helpText, MessageWindow::Buttons::Ok);
		};
		onClose = &OS::quit;

		Size desktopSize = Desktop::size();
		Size windowSize = layout.minimumGeometry().size();
		Position center = {
			(desktopSize.width  - windowSize.width ) / 2,
			(desktopSize.height - windowSize.height) / 2
		};
		setGeometry({center, windowSize});
		setVisible();
	}
	void setBusy(bool busy) {
		layout.setEnabled(!busy);
		OS::processEvents();
	}
};

int usage() {
	print("gcm-tool by jchadwick <johnwchadwick@gmail.com>\n");
	print("\n");
	print("Usage:\n");
	print("  gcm-tool                             # For GUI mode.\n");
	print("  gcm-tool <action> <input> <output>\n");
	print("\n");
	print("actions:\n");
	print("  unpack <in gcm file> <out directory>\n");
	print("  repack <in directory> <out gcm file>\n");
	print("\n");

	return 0;
}

int main(int argc, char **argv) {
	#if defined(PLATFORM_WINDOWS)
	utf8_args(argc, argv);
	#endif

	if(argv[1] && (!strcmp(argv[1], "--help") ||
	               !strcmp(argv[1], "-h")))
		return usage();

	if(argc == 4) {
		if(!strcmp(argv[1], "unpack")) {
			if(!unpack(argv[2], argv[3]))
				return print("Error: could not unpack ", argv[2], "\n"), 2;
		} else if(!strcmp(argv[1], "repack")) {
			if(!repack(argv[2], argv[3]))
				return print("Error: could not repack from", argv[2], "\n"), 3;
		} else	return print("Error: invalid action.\n"), 1;
		return 0;
	}

	Application *application = new Application(argc, argv);
	OS::main();
	delete application;

	return 0;
}
