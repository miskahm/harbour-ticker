Name:       harbour-ticker
Summary:    Live stock and index tickers on the cover
Version:    0.1.0
Release:    26
License:    GPL-3.0-only
URL:        https://github.com/miskahm/harbour-ticker
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Network)

%description
Native SailfishOS app whose cover shows live stock and index tickers.
No default tickers — add via Browse (45 curated) or free-text. Data
from Yahoo Finance (keyless), configurable refresh and cover layout.

%prep
%setup -q -n %{name}-%{version}

%build
%cmake
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%cmake_install

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}/qml
%{_datadir}/icons/hicolor/86x86/apps/%{name}.png
%{_datadir}/icons/hicolor/108x108/apps/%{name}.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/172x172/apps/%{name}.png
%{_datadir}/applications/%{name}.desktop
