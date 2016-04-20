
Summary: triggerd Tool
Name: triggerd
Version: 1.6
Release: 1%{?dist}
License: Distributable
Group: System Environment/Utilities
AutoReqProv: no

Packager: Nathan Neulinger <nneul@neulinger.org>

Source: triggerd-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
This contains the triggerd utility.

%prep
%setup -c -q -n triggerd

%build
cd triggerd-%{version}
make 

%install
cd triggerd-%{version}
make SBINDIR=%{buildroot}%{_sbindir} install

%post

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%attr(0755, root, root) %{_sbindir}/triggerd

%changelog
