<?php
// script criação de Tabelas de banco de dados 
$servername = "localhost";
$username = "root";
$password = "112004";
$dbname = "TrabalhoTCC";

$conn = mysqli_connect($servername, $username, $password, $dbname);

if (!$conn){
	die("Connection failed: ".mysqli_connect_error());
}

$sql = "CREATE TABLE responsavel (id INT(6) PRIMARY KEY,
nome VARCHAR(30) NOT NULL,
email VARCHAR(50),
reg_date TIMESTAMP)";

if (mysqli_query($conn,$sql)) { echo "Tabela responsavel foi criada com sucesso";
} else {
	echo "Erro na criação da tabela:".mysqli_error($conn);
}

?>