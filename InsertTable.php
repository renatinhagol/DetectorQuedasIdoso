<?php
// script inserir dados nas Tabelas de banco de dados 
$servername = "localhost";
$username = "root";
$password = "112004";
$dbname = "TrabalhoTCC";

$conn = mysqli_connect($servername, $username, $password, $dbname);

if (!$conn){
	die("Connection failed: ".mysqli_connect_error());
}

$sql = "INSERT INTO responsavel (id, nome, email) VALUES(1,'Renata Gomes ', 'renatinhagol@hotmail.com')";


if (mysqli_query($conn,$sql)) { echo "Novo registro incluído com sucesso";
} else {
	echo "Erro:".$sql. "<br>".mysqli_error($conn);
}
mysqli_close($conn);
?>