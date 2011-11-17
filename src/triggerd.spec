
Summary: triggerd Tool
Name: triggerd
Version: 1.2
Release: 1
License: Distributable
Group: System Environment/Utilities
AutoReqProv: no

Packager: Nathan Neulinger <nneul@mst.edu>

Source: triggerd.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
This contains the triggerd utility.

%prep
%setup -c -q -n triggerd

%build
cd triggerd
make 

%install
cd triggerd
make SBINDIR=%{buildroot}%{_sbindir} install

%post

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%attr(0755, root, root) %{_sbindir}/triggerd

%changelog
