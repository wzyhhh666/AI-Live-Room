net stop MySQL57
echo Stopping MySQL57... %ERRORLEVEL%

echo [Step 2] Starting MySQL with skip-grant-tables on port 3309...
start "MySQL-Reset" /B "D:\mysql_install\bin\mysqld.exe" --defaults-file="C:\ProgramData\MySQL\MySQL Server 5.7\my.ini" --skip-grant-tables --skip-networking --port=3309

timeout /t 4 /nobreak >nul

echo [Step 3] Connecting and resetting password...
"D:\mysql_install\bin\mysql.exe" -u root --skip-password -P 3309 -e "FLUSH PRIVILEGES; ALTER USER 'root'@'localhost' IDENTIFIED BY 'colin123'; FLUSH PRIVILEGES;" 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo First method failed, trying UPDATE method...
    "D:\mysql_install\bin\mysql.exe" -u root --skip-password -P 3309 -e "UPDATE mysql.user SET authentication_string=PASSWORD('colin123') WHERE User='root' AND Host='localhost'; FLUSH PRIVILEGES;" 2>&1
)

echo [Step 4] Shutting down temp MySQL...
"D:\mysql_install\bin\mysqladmin.exe" -u root --skip-password -P 3309 shutdown 2>&1
timeout /t 2 /nobreak >nul

echo [Step 5] Restarting MySQL57 service...
net start MySQL57
timeout /t 3 /nobreak >nul

echo [Step 6] Testing new password...
"D:\mysql_install\bin\mysql.exe" -u root -pcolin123 -P 3308 -e "SELECT 'MySQL Password Reset SUCCESS!' as Status, VERSION() as Version, USER() as CurrentUser;"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ======================================
    echo  SUCCESS! Password is now: colin123
    echo ======================================
) else (
    echo.
    echo ======================================
    echo  FAILED! Please run this script as Administrator.
    echo ======================================
)

pause
