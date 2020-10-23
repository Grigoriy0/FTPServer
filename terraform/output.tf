output "ftp_ip" {
  value = aws_instance.ftp-server.public_ip
}

output "ftp_priv_ip" {
  value = aws_instance.ftp-server.private_ip
}

output "db_priv_ip" {
  value = aws_db_instance.db_ftp.endpoint
}

output "db_name" {
  value = aws_db_instance.db_ftp.name
}

output "db_username" {
  value = aws_db_instance.db_ftp.username
}
