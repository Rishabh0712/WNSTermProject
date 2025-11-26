#!/bin/bash

# Fix rsyslog configuration in AMF-2 container
# Disables imklog (kernel logging) which is not available in containers

echo "Fixing rsyslog in AMF-2 container..."

# Stop any running rsyslog processes
docker exec rfsim5g-oai-amf-2 pkill rsyslogd 2>/dev/null || true

# Remove stale PID file
docker exec rfsim5g-oai-amf-2 rm -f /run/rsyslogd.pid

# Disable imklog module (kernel logging not available in containers)
docker exec rfsim5g-oai-amf-2 bash -c "cat > /etc/rsyslog.d/00-container.conf << 'EOF'
# Disable kernel logging in container environment
\$ModLoad imuxsock
# Do not load imklog - kernel logs not accessible in containers
EOF"

# Create a simplified rsyslog config that works in containers
docker exec rfsim5g-oai-amf-2 bash -c "cat > /etc/rsyslog.d/01-amf-local.conf << 'EOF'
# Local logging for AMF-2
\$ActionFileDefaultTemplate RSYSLOG_TraditionalFileFormat
*.info;mail.none;authpriv.none;cron.none    /var/log/messages
authpriv.*                                   /var/log/secure
mail.*                                       -/var/log/maillog
cron.*                                       /var/log/cron
*.emerg                                      :omusrmsg:*
uucp,news.crit                               /var/log/spooler
local7.*                                     /var/log/boot.log
EOF"

# Restart rsyslog with the new configuration
docker exec rfsim5g-oai-amf-2 rsyslogd -n &

echo "Done! rsyslog should now be running without errors."
echo ""
echo "Verify with:"
echo "  docker exec rfsim5g-oai-amf-2 ps aux | grep rsyslog"
