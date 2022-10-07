Name:		dftcalc
Version:	@VERSION
Release:	1%{?dist}
Summary:	A Dynamic Fault Tree calculator for reliability and availability

License:	MIT
URL:		https://github.com/utwente-fmt/dftcalc
Source0:	%{name}-%{version}.tar.xz

BuildRequires:	cmake gcc-c++, flex-devel >= 2.6.3, flex >= 2.6.3, bison >= 3.3, bc
BuildRequires:	gsl-devel >= 2.6, yaml-cpp-devel >= 0.6.3
BuildRequires:	storm >= 1.6.2, dftres >= 1.0.2
Requires:	storm >= 1.6.2, dftres >= 1.0.2

%description
A Dynamic Fault Tree calculator for reliability and availability.

%prep
%setup -q

%build
%cmake -DDFTROOT=%{_prefix}
%cmake_build

%check
cd test && PATH="$PATH:$(pwd)/../bin" DFT2LNTROOT=$PWD/.. HOME=/tmp sh test.sh

%install
%cmake_install
rm %{buildroot}%{_prefix}/include/dft2lnt/dft2lnt.h
rmdir %{buildroot}%{_prefix}/include/dft2lnt

%files
%license LICENSE
%dir %{_prefix}/share/dft2lnt
%dir %{_prefix}/share/dft2lnt/tests
%{_prefix}/share/dft2lnt/tests/*
%{_bindir}/dftcalc
%{_bindir}/dft2lntc
%{_bindir}/dfttest
%{_libdir}/libdft2lnt.so
