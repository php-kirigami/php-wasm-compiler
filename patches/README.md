# patches/

Convention (inspired by `patches/` + `create-patches.bat` in
[php-static-autobuilder](https://github.com/ZmotriN/php-static-autobuilder)):
one subfolder per library or per PHP version, holding `.patch` files applied
automatically during the build, so a one-off compilation hack no longer
means hand-editing a Dockerfile.

```
patches/
  php-8.5/*.patch        # already partly the case: compile/php/php8.5.patch
  libcurl/*.patch
  oniguruma/*.patch
  ...
```

**Status: not wired up yet.** `compile/php/Dockerfile` already has its own
per-version patch mechanism (`php*.patch`, see `git apply` in the
Dockerfile). The third-party lib Dockerfiles (`compile/lib*/Dockerfile`),
on the other hand, apply their compilation hacks inline (`sed`/`replace.sh`)
— nothing reads this folder yet. Still to do: add, in each lib Dockerfile, a
`COPY ./patches/<lib>/ ...` + `git apply`/`patch` step before
`configure`/`make`, the same way `apply-mysqlnd-patch.sh` does for PHP.
