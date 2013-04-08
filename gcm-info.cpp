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

string errorStr;
const string Title = "GCM Info Tool";

string getinfo(string fn) {
	gamecube::gcm iso;

	if(!iso.open(new filestream(fn)))
		return "Unable to open and parse disk image.";

	return {
		"File: ", fn, "\n",
		"\n",
		"Game code: ", string((const char *)iso.header.gameCode), "\n",
		"Revision: ", nall::decimal(iso.header.version), "\n",
		"Title: ", string((const char *)iso.header.gameName)
	};
}

struct Application : Window {
	HorizontalLayout layout;
	Label label;
	Button infoButton;
	Button helpButton;

	Application(int argc, char **argv) {
		string basepath = dir(realpath(argv[0]));

		setTitle(Title);
		setResizable(false);

		label.setText("Actions: ");
		layout.append(label, { 0, 0 }, 5);
		infoButton.setText("select rom");
		layout.append(infoButton, { 0, 0 }, 5);
		helpButton.setText("help");
		layout.append(helpButton, { 0, 0 }, 5);
		append(layout);

		infoButton.onActivate = [&]() {
			string fn;
			fn = DialogWindow::fileOpen(*this, "", "Gamecube ISO (*.iso)", "Gamecube GCM (*.gcm)");
			if(!fn.empty())
				MessageWindow::information(*this, getinfo(fn), MessageWindow::Buttons::Ok);
		};

		helpButton.onActivate = [&]() {
			string helpText = {
				Title, " by jchadwick <johnwchadwick@gmail.com>\n",
				"Shows basic information about a GCM.\n",
				"\n"
				"Uses byuu's nall and phoenix libraries."
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
};

int main(int argc, char **argv) {
	#if defined(PLATFORM_WINDOWS)
	utf8_args(argc, argv);
	#endif

	Application *application = new Application(argc, argv);
	OS::main();
	delete application;

	return 0;
}
